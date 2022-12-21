#include "hgpch.h"
#include "Shader.h"


#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_reflect.h>
#include <volk.h>
#include <yaml-cpp/yaml.h>

#include "Hog/Renderer/Renderer.h"
#include "Hog/Core/Timer.h"
#include "Hog/Core/CVars.h"
#include "Hog/Utils/Filesystem.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Debug/Instrumentor.h"

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
		HG_PROFILE_FUNCTION();

		const std::string cacheDirectory(CVar_ShaderCacheDir.Get());
		if (!std::filesystem::exists(cacheDirectory))
			std::filesystem::create_directories(cacheDirectory);
	}

	Ref<ShaderSource> ShaderSource::Deserialize(YAML::Node& info)
	{
		HG_PROFILE_FUNCTION();

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
		HG_PROFILE_FUNCTION();

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
		HG_PROFILE_FUNCTION();

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

	void ShaderCache::CleanupImpl()
	{
		HG_PROFILE_FUNCTION();

		SaveToFilesystem();
	}

	Ref<ShaderSource> ShaderCache::GetShaderImpl(const std::string& name)
	{
		HG_PROFILE_FUNCTION();

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
		HG_PROFILE_FUNCTION();

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
		HG_PROFILE_FUNCTION();

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
		HG_PROFILE_FUNCTION();

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

	ShaderReflection::ReflectionData ShaderReflection::ReflectPipelineLayout(const std::unordered_map<ShaderType, Ref<ShaderSource>>& sources)
	{
		HG_PROFILE_FUNCTION();

		ShaderReflection::ReflectionData data{};
		for (const auto& [stage, source] : sources)
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

					if (data.DescriptorSetLayoutBinding[refl_set.set].size() < refl_binding.binding + 1)
					{
						data.DescriptorSetLayoutBinding[refl_set.set].resize(refl_binding.binding + 1);
					}

					VkDescriptorSetLayoutBinding& layoutBinding = data.DescriptorSetLayoutBinding[refl_set.set][refl_binding.binding];
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

				data.PushConstantRanges.push_back(pushConstant);
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

				data.VertexInputAttributeDescriptions.reserve(inputVariables.size());
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
					data.VertexInputAttributeDescriptions.push_back(attr_desc);
				}
				// Sort attributes by location
				std::sort(std::begin(data.VertexInputAttributeDescriptions), std::end(data.VertexInputAttributeDescriptions),
					[](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
						return a.location < b.location; });
				// Compute final offsets of each attribute, and total vertex stride.
				for (auto& attribute : data.VertexInputAttributeDescriptions) {
					uint32_t format_size = DataType(attribute.format).TypeSize();
					attribute.offset = bindingDescription.stride;
					bindingDescription.stride += format_size;
				}
				// Nothing further is done with attribute_descriptions or binding_description
				// in this sample. A real application would probably derive this information from its
				// mesh format(s); a similar mechanism could be used to ensure mesh/shader compatibility.
				if (!data.VertexInputAttributeDescriptions.empty())
				{
					data.VertexInputBindingDescriptions.push_back(bindingDescription);
				}
			}

			spvReflectDestroyShaderModule(&spvmodule);
		}

		for (int i = 0; i < data.DescriptorSetLayoutBinding.size(); ++i)
		{
			std::vector<VkDescriptorBindingFlags> bindingFlags(data.DescriptorSetLayoutBinding[i].size(), VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);

			VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlags = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = static_cast<uint32_t>(bindingFlags.size()),
				.pBindingFlags = bindingFlags.data(),
			};

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = &layoutBindingFlags;
			layoutInfo.bindingCount = (uint32_t)data.DescriptorSetLayoutBinding[i].size();
			layoutInfo.pBindings = data.DescriptorSetLayoutBinding[i].data();

			data.DescriptorSetLayouts[i] = Renderer::GetDescriptorLayoutCache()->CreateDescriptorLayout(&layoutInfo);
			// CheckVkResult(vkCreateDescriptorSetLayout(GraphicsContext::GetDevice(), &layoutInfo, nullptr, &data.DescriptorSetLayouts[i]));
		}

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

		PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)data.PushConstantRanges.size();
		PipelineLayoutCreateInfo.pPushConstantRanges = data.PushConstantRanges.data();
		PipelineLayoutCreateInfo.setLayoutCount = (uint32_t)data.DescriptorSetLayouts.size(); // Optional
		PipelineLayoutCreateInfo.pSetLayouts = data.DescriptorSetLayouts.data(); // Optional

		CheckVkResult(vkCreatePipelineLayout(GraphicsContext::GetDevice(), &PipelineLayoutCreateInfo, nullptr, &data.PipelineLayout));

		return data;
	}
}
