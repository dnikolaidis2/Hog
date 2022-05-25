#include "hgpch.h"
#include "Shader.h"


#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_reflect.h>
#include <vulkan/vulkan.h>
#include <yaml-cpp/yaml.h>

#include "Hog/Core/Timer.h"
#include "Hog/Core/CVars.h"
#include "Hog/Utils/Filesystem.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Renderer/GraphicsContext.h"

AutoCVar_String CVar_ShaderCacheDBFile("shader.cacheDBFile", "Shader cache database filename", ".db", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderCacheDir("shader.cachePath", "Shader cache directory", "assets/cache/shader/vulkan", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderSourceDir("shader.sourceDir", "Shader source directory", "assets/shaders/", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderMacroDef("shader.compilation.macros", "Definition string for all macros in shader", "", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderCompiledFileExtension("shader.compiledFileExtension", "Shader compiled file extension", ".spv", CVarFlags::EditReadOnly);
AutoCVar_Int	CVar_ShaderOptimizationLevel("shader.compilation.optimizationLevel",
	"Shader compilation optimization level. 0 zero optimization, 1 optimize for size, 2 optimize for performance",
	0, CVarFlags::None);

namespace Hog {

	static void CreateCacheDirectoryIfNeeded()
	{
		const std::string cacheDirectory(CVar_ShaderCacheDir.Get());
		if (!std::filesystem::exists(cacheDirectory))
			std::filesystem::create_directories(cacheDirectory);
	}

	Ref<ShaderSource> ShaderSource::Deserialize(YAML::Node& info)
	{
		std::string name = info["Name"].as<std::string>();
		std::filesystem::path filepath = std::filesystem::path(info["FilePath"].as<std::string>());
		std::filesystem::path cacheFilepath = std::filesystem::path(info["CacheFilePath"].as<std::string>());
		ShaderType type((info["Type"].as<std::string>()));
		size_t hash = info["Hash"].as<size_t>();

		std::vector<uint32_t> code = ReadBinaryFile(cacheFilepath.string());

		auto reference = CreateRef<ShaderSource>(std::forward<std::string>(name),
			std::forward<std::filesystem::path>(filepath),
			type, hash, std::forward<std::vector<uint32_t>>(code));
		reference->CacheFilePath = cacheFilepath;

		return reference;
	}

	void ShaderSource::Serialize(YAML::Emitter& emitter)
	{
		CacheFilePath = std::filesystem::path(CVar_ShaderCacheDir.Get()) /
			std::filesystem::path(Name + CVar_ShaderCompiledFileExtension.Get());

		emitter << YAML::Key << Name << YAML::BeginMap;
		{
			emitter << YAML::Key << "Name" << YAML::Value << Name;
			emitter << YAML::Key << "FilePath" << YAML::Value << FilePath.string();
			emitter << YAML::Key << "CacheFilePath" << YAML::Value << CacheFilePath.string();
			emitter << YAML::Key << "Type" << YAML::Value << static_cast<std::string>(Type);
			emitter << YAML::Key << "Hash" << YAML::Value << Hash;
		}

		emitter << YAML::EndMap;

		WriteBinaryFile(CacheFilePath.string(), Code);
	}

	void ShaderCache::InitializeImpl()
	{
		auto cacheDir = std::filesystem::path(CVar_ShaderCacheDir.Get());
		auto dbFilename = std::filesystem::path(CVar_ShaderCacheDBFile.Get());
		auto dbFile = cacheDir / dbFilename;

		if (!std::filesystem::exists(cacheDir))
		{
			HG_CORE_INFO("Could not find shader cache directory");
			return;
		}

		if (!std::filesystem::exists(dbFile))
		{
			HG_CORE_INFO("Could not find shader cache database file");
			return;
		}

		// Read shader cache db file
		// Populate internal cache
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(dbFile.string());
		}
		catch (YAML::ParserException e)
		{
			HG_CORE_WARN("Could not parse shader database file");
			return;
		}

		auto shaders = data["ShaderCache"];
		if (shaders)
		{
			for (auto shader : shaders)
			{
				auto name = shader.first.as<std::string>();
				auto info = data["ShaderCache"][name.c_str()];
				auto shaderSource = ShaderSource::Deserialize(info);
				m_ShaderCache.insert({ name, shaderSource });
			}
		}
	}

	void ShaderCache::DeinitializeImpl()
	{
		SaveToFilesystem();
	}

	Ref<ShaderSource> ShaderCache::GetShaderImpl(const std::string& name)
	{
		// Read shader file
		const std::filesystem::path shaderDir(CVar_ShaderSourceDir.Get());
		const std::filesystem::path file(name);
		std::filesystem::path fullPath = shaderDir / file;

		if (!std::filesystem::exists(fullPath))
		{
			HG_CORE_ERROR("Could find file while trying to load shader {0}", fullPath);
			return nullptr;
		}


		if (!file.has_extension())
		{
			HG_CORE_ERROR("Shaders must have appropriate file extension");
			return nullptr;
		}

		std::string source = ReadFile(fullPath);
		ShaderType type(file.extension().string().substr(1));

		// Hash shader file
		std::size_t hash = std::hash<std::string>{}(source);

		// Check if shader is known by cache
		if (m_ShaderCache.contains(name))
		{
			// if yes check if its hash is up to date
			if (m_ShaderCache[name]->Hash == hash)
			{
				// if yes return ShaderSource
				return m_ShaderCache[name];
			}
		}

		// else compile shader, update cache, save, return new ShaderSource
		auto code = CompileShader(source, type, fullPath);
		auto shaderSource = ShaderSource::Create(std::forward<std::string>(static_cast<std::string>(name)),
			std::forward<std::filesystem::path>(fullPath),
			type, hash, std::forward<std::vector<uint32_t>>(code));

		m_ShaderCache[name] = shaderSource;
		SaveToFilesystem();

		return shaderSource;
	}

	Ref<ShaderSource> ShaderCache::ReloadShaderImpl(const std::string& name)
	{
		// Read shader file
		const std::filesystem::path shaderDir(CVar_ShaderCacheDir.Get());
		const std::filesystem::path file(name);
		std::filesystem::path fullPath = shaderDir / file;

		if (!std::filesystem::exists(fullPath))
		{
			HG_CORE_ERROR("Could find file while trying to load shader {0}", fullPath);
			return nullptr;
		}


		if (!file.has_extension())
		{
			HG_CORE_ERROR("Shaders must have appropriate file extension");
			return nullptr;
		}

		std::string source = ReadFile(fullPath);
		ShaderType type(file.extension().string().substr(1));

		// Hash shader file
		std::size_t hash = std::hash<std::string>{}(source);

		// Compile shader
		auto code = CompileShader(source, type, fullPath);
		auto shaderSource = ShaderSource::Create(std::forward<std::string>(static_cast<std::string>(name)),
			std::forward<std::filesystem::path>(fullPath),
			type, hash, std::forward<std::vector<uint32_t>>(code));

		// insert new ShaderSource to cache, save
		m_ShaderCache[name] = shaderSource;
		SaveToFilesystem();

		// return new ShaderSource
		return shaderSource;
	}

	void ShaderCache::SaveToFilesystem()
	{
		CreateCacheDirectoryIfNeeded();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderCache" << YAML::Value << YAML::BeginMap;
		for (auto entry : m_ShaderCache)
		{
			entry.second->Serialize(out);
		}

		out << YAML::EndMap;
		out << YAML::EndMap;

		auto cacheDir = std::filesystem::path(CVar_ShaderCacheDir.Get());
		auto dbFilename = std::filesystem::path(CVar_ShaderCacheDBFile.Get());
		auto dbFile = cacheDir / dbFilename;
		std::ofstream fout(dbFile);
		fout << out.c_str();
	}

	std::vector<uint32_t> ShaderCache::CompileShader(const std::string& source, ShaderType type, const std::filesystem::path& filepath)
	{
		Timer timer;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

		std::stringstream macroDefs(CVar_ShaderMacroDef.Get());
		std::string macro;
		std::vector<std::string> macros;
		while(std::getline(macroDefs, macro, ';'))
		{
			macros.push_back(macro);
		}

		for (const auto & macroDef : macros)
		{
			std::string name, value;
			std::stringstream stream(macroDef);
			std::getline(stream, name, '=');
			std::getline(stream, value, '=');
			options.AddMacroDefinition(name.c_str(), name.size(),
				value.c_str(), value.size());
		}

		if (CVar_ShaderOptimizationLevel.Get() == 0)
		{
			options.SetOptimizationLevel(shaderc_optimization_level_zero);
		}
		else if (CVar_ShaderOptimizationLevel.Get() == 1)
		{
			options.SetOptimizationLevel(shaderc_optimization_level_size);
		}
		else if (CVar_ShaderOptimizationLevel.Get() == 2)
		{
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
		}

		std::vector<uint32_t> shaderData;

		// Cache did not contain an up to date version of the shader
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, type, filepath.string().c_str(), options);
		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			HG_CORE_ERROR(module.GetErrorMessage());
			HG_CORE_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
			return shaderData;
		}

		shaderData = std::vector<uint32_t>(module.cbegin(), module.cend());

		HG_CORE_WARN("Shader creation took {0} ms", timer.ElapsedMillis());

		return shaderData;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertex, const std::string& fragment)
	{
		auto shader = CreateRef<Shader>(name);
		shader->AddStage(vertex);
		shader->AddStage(fragment);

		return shader;
	}

	Ref<Shader> Shader::Create(const std::string& name)
	{
		auto shader = CreateRef<Shader>(name);
		shader->AddStage(name);

		return shader;
	}

	Shader::~Shader()
	{
		for (auto& layout : m_DescriptorSetLayouts)
		{
			vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice(), layout, nullptr);
		}

		vkDestroyPipelineLayout(GraphicsContext::GetDevice(), m_PipelineLayout, nullptr);

		for (auto& [stage, shaderModule] : m_Modules)
		{
			vkDestroyShaderModule(GraphicsContext::GetDevice(), shaderModule, nullptr);
		}
		
		m_Sources.clear();
	}

	void Shader::AddStage(Ref<ShaderSource> source)
	{
		if (m_Sources.find(source->Type) != m_Sources.end())
		{
			HG_CORE_WARN("Replacing existing source in shader!");
		}

		m_Sources[source->Type] = source;
	}

	void Shader::AddStage(const std::string& name)
	{
		Ref<ShaderSource> source = ShaderCache::GetShader(name);
		AddStage(source);
	}

	void Shader::Generate(VkSpecializationInfo specializationInfo)
	{
		ReflectPipelineLayout();


		m_Pipeline = Pipeline::CreateCompute(m_PipelineLayout);

		for (const auto& [stage, source] : m_Sources)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = (uint32_t)source->Code.size() * sizeof(uint32_t);
			createInfo.pCode = source->Code.data();

			VkShaderModule shaderModule;
			CheckVkResult(vkCreateShaderModule(GraphicsContext::GetDevice(), &createInfo, nullptr, &shaderModule));

			m_Pipeline->AddShaderStage(stage, shaderModule, &specializationInfo);
			m_Modules[stage] = shaderModule;
		}


		m_Pipeline->Create();
	}

	void Shader::Generate(VkRenderPass renderPass, VkSpecializationInfo specializationInfo)
	{
		ReflectPipelineLayout();

		m_Pipeline = Pipeline::CreateGraphics(m_VertexInputBindingDescriptions, m_VertexInputAttributeDescriptions, m_PipelineLayout, renderPass);

		for (const auto& [stage, source] : m_Sources)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = (uint32_t)source->Code.size() * sizeof(uint32_t);
			createInfo.pCode = source->Code.data();

			VkShaderModule shaderModule;
			CheckVkResult(vkCreateShaderModule(GraphicsContext::GetDevice(), &createInfo, nullptr, &shaderModule));

			m_Pipeline->AddShaderStage(stage, shaderModule, &specializationInfo);
			m_Modules[stage] = shaderModule;
		}


		m_Pipeline->Create();
	}

	void Shader::Bind(VkCommandBuffer commandBuffer)
	{
		m_Pipeline->Bind(commandBuffer);
	}

	void Shader::Reaload()
	{
	}

	void Shader::ReflectPipelineLayout()
	{
		for (const auto& [stage, source] : m_Sources)
		{
			SpvReflectShaderModule spvmodule;
			SpvReflectResult result = spvReflectCreateShaderModule(source->Code.size() * sizeof(uint32_t), source->Code.data(), &spvmodule);

			uint32_t count = 0;
			result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, NULL);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			std::vector<SpvReflectDescriptorSet*> sets(count);
			result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, sets.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);
			for (size_t i_set = 0; i_set < sets.size(); ++i_set)
			{
				const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
				for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) 
				{
					const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);

					if (m_DescriptorSetLayoutBinding[refl_set.set].size() < refl_binding.binding + 1)
					{
						m_DescriptorSetLayoutBinding[refl_set.set].resize(refl_binding.binding + 1);
					}

					VkDescriptorSetLayoutBinding& layoutBinding = m_DescriptorSetLayoutBinding[refl_set.set][refl_binding.binding];
					layoutBinding.binding = refl_binding.binding;
					layoutBinding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
					layoutBinding.descriptorCount = 1;
					for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
						layoutBinding.descriptorCount *= refl_binding.array.dims[i_dim];
					}
					layoutBinding.stageFlags |= static_cast<VkShaderStageFlags>(spvmodule.shader_stage);
				}
			}

			result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, NULL);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			std::vector<SpvReflectBlockVariable*> pconstants(count);
			result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, pconstants.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			for (size_t i_const = 0; i_const < pconstants.size(); ++i_const)
			{
				VkPushConstantRange pushConstant = {};
				pushConstant.offset = pconstants[i_const]->offset;
				pushConstant.size = pconstants[i_const]->size;
				pushConstant.stageFlags |= static_cast<VkShaderStageFlags>(stage);

				m_PushConstantRanges.push_back(pushConstant);
			}

			if (stage == ShaderType::Defaults::Vertex)
			{
				result = spvReflectEnumerateInputVariables(&spvmodule, &count, NULL);
				assert(result == SPV_REFLECT_RESULT_SUCCESS);

				std::vector<SpvReflectInterfaceVariable*> inputVariables(count);
				result = spvReflectEnumerateInputVariables(&spvmodule, &count, inputVariables.data());
				assert(result == SPV_REFLECT_RESULT_SUCCESS);

				VkVertexInputBindingDescription bindingDescription = {};
				bindingDescription.binding = 0;
				bindingDescription.stride = 0;  // computed below
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				m_VertexInputAttributeDescriptions.reserve(inputVariables.size());
				for (size_t i_var = 0; i_var < inputVariables.size(); ++i_var) {
					const SpvReflectInterfaceVariable& refl_var = *(inputVariables[i_var]);
					// ignore built-in variables
					if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
						continue;
					}
					VkVertexInputAttributeDescription attr_desc{};
					attr_desc.location = refl_var.location;
					attr_desc.binding = bindingDescription.binding;
					attr_desc.format = static_cast<VkFormat>(refl_var.format);
					attr_desc.offset = 0;  // final offset computed below after sorting.
					m_VertexInputAttributeDescriptions.push_back(attr_desc);
				}
				// Sort attributes by location
				std::sort(std::begin(m_VertexInputAttributeDescriptions), std::end(m_VertexInputAttributeDescriptions),
					[](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
						return a.location < b.location; });
				// Compute final offsets of each attribute, and total vertex stride.
				for (auto& attribute : m_VertexInputAttributeDescriptions) {
					uint32_t format_size = DataType(attribute.format).TypeSize();
					attribute.offset = bindingDescription.stride;
					bindingDescription.stride += format_size;
				}
				// Nothing further is done with attribute_descriptions or binding_description
				// in this sample. A real application would probably derive this information from its
				// mesh format(s); a similar mechanism could be used to ensure mesh/shader compatibility.
				if (!m_VertexInputAttributeDescriptions.empty())
				{
					m_VertexInputBindingDescriptions.push_back(bindingDescription);
				}
			}

			spvReflectDestroyShaderModule(&spvmodule);
		}

		for (int i = 0; i < m_DescriptorSetLayoutBinding.size(); ++i)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = (uint32_t)m_DescriptorSetLayoutBinding[i].size();
			layoutInfo.pBindings = m_DescriptorSetLayoutBinding[i].data();

			CheckVkResult(vkCreateDescriptorSetLayout(GraphicsContext::GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayouts[i]));
		}

		m_PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)m_PushConstantRanges.size();
		m_PipelineLayoutCreateInfo.pPushConstantRanges = m_PushConstantRanges.data();
		m_PipelineLayoutCreateInfo.setLayoutCount = (uint32_t)m_DescriptorSetLayouts.size(); // Optional
		m_PipelineLayoutCreateInfo.pSetLayouts = m_DescriptorSetLayouts.data(); // Optional

		CheckVkResult(vkCreatePipelineLayout(GraphicsContext::GetDevice(), &m_PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
	}
}
