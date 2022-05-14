#include "hgpch.h"
#include "Shader.h"

#include "Hog/Core/Timer.h"
#include "Hog/Core/CVars.h"
#include "Hog/Utils/Filesystem.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Renderer/GraphicsContext.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <vulkan/vulkan.h>
#include <yaml-cpp/yaml.h>

#include "Constants.h"

static auto& context = Hog::GraphicsContext::Get();

AutoCVar_String CVar_ShaderCacheDBFile("shader.cacheDBFile", "Shader cache database filename", ".db", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderCacheDir("shader.cachePath", "Shader cache directory", "assets/cache/shader/vulkan", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderSourceDir("shader.sourceDir", "Shader source directory", "assets/shaders/", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderCompiledFileExtension("shader.compiledFileExtension", "Shader compiled file extension", ".spv", CVarFlags::EditReadOnly);
AutoCVar_Int	CVar_ShaderOptimizationLevel("shader.optimizationLevel",
	"Shader compilation optimization level. 0 zero optimization, 1 optimize for size, 2 optimize for performance",
	0, CVarFlags::None);

namespace Hog {
	namespace Utils {
		static VkFormat SpirvBaseTypeToVkFormat(const spirv_cross::SPIRType& type)
		{
			switch (type.basetype) {
				case spirv_cross::SPIRType::SByte:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R8_SINT;
						case 2: return VK_FORMAT_R8G8_SINT;
						case 3: return VK_FORMAT_R8G8B8_SINT;
						case 4: return VK_FORMAT_R8G8B8A8_SINT;
					}
					break;
				case spirv_cross::SPIRType::UByte:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R8_UINT;
						case 2: return VK_FORMAT_R8G8_UINT;
						case 3: return VK_FORMAT_R8G8B8_UINT;
						case 4: return VK_FORMAT_R8G8B8A8_UINT;
					}
					break;
				case spirv_cross::SPIRType::Short:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R16_SINT;
						case 2: return VK_FORMAT_R16G16_SINT;
						case 3: return VK_FORMAT_R16G16B16_SINT;
						case 4: return VK_FORMAT_R16G16B16A16_SINT;
					}
					break;
				case spirv_cross::SPIRType::UShort:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R16_UINT;
						case 2: return VK_FORMAT_R16G16_UINT;
						case 3: return VK_FORMAT_R16G16B16_UINT;
						case 4: return VK_FORMAT_R16G16B16A16_UINT;
					}
					break;
				case spirv_cross::SPIRType::Half:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R16_SFLOAT;
						case 2: return VK_FORMAT_R16G16_SFLOAT;
						case 3: return VK_FORMAT_R16G16B16_SFLOAT;
						case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
					}
					break;
				case spirv_cross::SPIRType::Int:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R32_SINT;
						case 2: return VK_FORMAT_R32G32_SINT;
						case 3: return VK_FORMAT_R32G32B32_SINT;
						case 4: return VK_FORMAT_R32G32B32A32_SINT;
					}
					break;
				case spirv_cross::SPIRType::UInt:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R32_UINT;
						case 2: return VK_FORMAT_R32G32_UINT;
						case 3: return VK_FORMAT_R32G32B32_UINT;
						case 4: return VK_FORMAT_R32G32B32A32_UINT;
					}
					break;
				case spirv_cross::SPIRType::Float:
					switch (type.vecsize) {
						case 1: return VK_FORMAT_R32_SFLOAT;
						case 2: return VK_FORMAT_R32G32_SFLOAT;
						case 3: return VK_FORMAT_R32G32B32_SFLOAT;
						case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
					}
					break;
				default:
					break;
			}
			return VK_FORMAT_UNDEFINED;
		}

		static VkShaderStageFlagBits ShaderStageFlagFromShaderKind(shaderc_shader_kind kind)
		{
			switch(kind)
			{
				case shaderc_glsl_vertex_shader:		return VK_SHADER_STAGE_VERTEX_BIT;
				case shaderc_glsl_fragment_shader:		return VK_SHADER_STAGE_FRAGMENT_BIT;
				case shaderc_glsl_compute_shader:		return VK_SHADER_STAGE_COMPUTE_BIT;
				case shaderc_glsl_anyhit_shader:		return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				case shaderc_glsl_raygen_shader:		return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				case shaderc_glsl_intersection_shader:	return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				case shaderc_glsl_miss_shader:			return VK_SHADER_STAGE_MISS_BIT_KHR;
				case shaderc_glsl_closesthit_shader:	return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				case shaderc_glsl_mesh_shader:			return VK_SHADER_STAGE_MESH_BIT_NV;
			}

			HG_CORE_ASSERT(false, "Unknown shader type!");
			return (VkShaderStageFlagBits)0;
		}

		VkShaderStageFlagBits ShaderTypeToShaderStageFlag(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex: 		return VK_SHADER_STAGE_VERTEX_BIT;
				case ShaderType::Fragment: 		return VK_SHADER_STAGE_FRAGMENT_BIT;
				case ShaderType::Compute: 		return VK_SHADER_STAGE_COMPUTE_BIT;
				case ShaderType::AnyHit: 		return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				case ShaderType::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				case ShaderType::Intersection: 	return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				case ShaderType::Miss: 			return VK_SHADER_STAGE_MISS_BIT_KHR;
				case ShaderType::ClosestHit: 	return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				case ShaderType::Mesh: 			return VK_SHADER_STAGE_MESH_BIT_NV;
			}

			return (VkShaderStageFlagBits)0;
		}

		static shaderc_shader_kind ShaderTypeToShaderKind(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex: 		return shaderc_glsl_vertex_shader;
				case ShaderType::Fragment: 		return shaderc_glsl_fragment_shader;
				case ShaderType::Compute: 		return shaderc_glsl_compute_shader;
				case ShaderType::AnyHit: 		return shaderc_glsl_anyhit_shader;
				case ShaderType::RayGeneration: return shaderc_glsl_raygen_shader;
				case ShaderType::Intersection: 	return shaderc_glsl_intersection_shader;
				case ShaderType::Miss: 			return shaderc_glsl_miss_shader;
				case ShaderType::ClosestHit: 	return shaderc_glsl_closesthit_shader;
				case ShaderType::Mesh: 			return shaderc_glsl_mesh_shader;
			}

			HG_CORE_ASSERT(false, "Unknown shader type!");
			return (shaderc_shader_kind)0;
		}

		static const char* ShaderTypeToString(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex: 		return "Vertex";
				case ShaderType::Fragment: 		return "Fragment";
				case ShaderType::Compute: 		return "Compute";
				case ShaderType::AnyHit: 		return "AnyHit";
				case ShaderType::RayGeneration: return "RayGeneration";
				case ShaderType::Intersection: 	return "Intersection";
				case ShaderType::Miss: 			return "Miss";
				case ShaderType::ClosestHit: 	return "ClosestHit";
				case ShaderType::Mesh: 			return "Mesh";
			}

			HG_CORE_ASSERT(false, "Unknown shader type!");
			return nullptr;
		}

		static ShaderType StringToShaderType(const std::string& str)
		{
			if (str == "Vertex") 		return ShaderType::Vertex;
			if (str == "Fragment") 		return ShaderType::Fragment;
			if (str == "Compute")		return ShaderType::Compute;
			if (str == "AnyHit")		return ShaderType::AnyHit;
			if (str == "RayGeneration")	return ShaderType::RayGeneration;
			if (str == "Intersection")	return ShaderType::Intersection;
			if (str == "Miss")			return ShaderType::Miss;
			if (str == "ClosestHit")	return ShaderType::ClosestHit;
			if (str == "Mesh")			return ShaderType::Mesh;

			HG_CORE_ASSERT(false, "Unknown shader type!");
			return (ShaderType)0;
		}

		static ShaderType ShaderKindToShaderType(shaderc_shader_kind kind)
		{
			switch (kind)
			{
				case shaderc_glsl_vertex_shader: 		return ShaderType::Vertex;
				case shaderc_glsl_fragment_shader: 		return ShaderType::Fragment;
				case shaderc_glsl_compute_shader: 		return ShaderType::Compute;
				case shaderc_glsl_anyhit_shader: 		return ShaderType::AnyHit;
				case shaderc_glsl_raygen_shader: 		return ShaderType::RayGeneration;
				case shaderc_glsl_intersection_shader: 	return ShaderType::Intersection;
				case shaderc_glsl_miss_shader: 			return ShaderType::Miss;
				case shaderc_glsl_closesthit_shader: 	return ShaderType::ClosestHit;
				case shaderc_glsl_mesh_shader: 			return ShaderType::Mesh;
			}

			HG_CORE_ASSERT(false, "Unknown shader type!");
			return (ShaderType)0;
		}

		static ShaderType FileExtensionToShaderType(const std::string& extension)
		{
			std::string string = extension.substr(1);
			if (string == "vertex")
				return ShaderType::Vertex;
			if (string == "fragment")
				return ShaderType::Fragment;
			if (string == "compute")
				return ShaderType::Compute;
			if (string == "anyhit")
				return ShaderType::AnyHit;
			if (string == "raygen")
				return ShaderType::RayGeneration;
			if (string == "intersection")
				return ShaderType::Intersection;
			if (string == "miss")
				return ShaderType::Miss;
			if (string == "closesthit")
				return ShaderType::ClosestHit;
			if (string == "mesh")
				return ShaderType::Mesh;

			HG_CORE_ASSERT(false, "Unknown shader type!");
			return (ShaderType)0;
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			const std::string cacheDirectory(CVar_ShaderCacheDir.Get());
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}
	}

	Ref<ShaderSource> ShaderSource::Deserialize(YAML::Node& info)
	{
		std::string name = info["Name"].as<std::string>();
		std::filesystem::path filepath = std::filesystem::path(info["FilePath"].as<std::string>());
		std::filesystem::path cacheFilepath = std::filesystem::path(info["CacheFilePath"].as<std::string>());
		ShaderType type = Utils::StringToShaderType(info["Type"].as<std::string>());
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
			emitter << YAML::Key << "Type" << YAML::Value << Utils::ShaderTypeToString(Type);
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
		ShaderType type = Utils::FileExtensionToShaderType(file.extension().string());

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
		ShaderType type = Utils::FileExtensionToShaderType(file.extension().string());

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
		Utils::CreateCacheDirectoryIfNeeded();

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

		options.AddMacroDefinition(MATERIAL_ARRAY_SIZE_NAMESTR, sizeof(MATERIAL_ARRAY_SIZE_NAMESTR) - 1,
			MATERIAL_ARRAY_SIZE_VALUESTR, sizeof(MATERIAL_ARRAY_SIZE_VALUESTR) - 1);
		options.AddMacroDefinition(TEXTURE_ARRAY_SIZE_NAMESTR, sizeof(TEXTURE_ARRAY_SIZE_NAMESTR) - 1,
			TEXTURE_ARRAY_SIZE_VALUESTR, sizeof(TEXTURE_ARRAY_SIZE_VALUESTR) - 1);

		
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
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::ShaderTypeToShaderKind(type), filepath.string().c_str(), options);
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
		auto shader = Create(name);
		shader->AddStage(vertex);
		shader->AddStage(fragment);
		shader->Generate();

		return shader;
	}

	Ref<Shader> Shader::Create(const std::string& name)
	{
		return CreateRef<Shader>(name);
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

	void Shader::Generate()
	{
		Reflect();

		for (int i = 0; i < m_DescriptorSetLayoutBinding.size(); ++i)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = (uint32_t)m_DescriptorSetLayoutBinding[i].size();
			layoutInfo.pBindings = m_DescriptorSetLayoutBinding[i].data();

			CheckVkResult(vkCreateDescriptorSetLayout(context.Device, &layoutInfo, nullptr, &m_DescriptorSetLayouts[i]));
		}

		m_PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)m_PushConstantRanges.size();
		m_PipelineLayoutCreateInfo.pPushConstantRanges = m_PushConstantRanges.data();
		m_PipelineLayoutCreateInfo.setLayoutCount = (uint32_t)m_DescriptorSetLayouts.size(); // Optional
		m_PipelineLayoutCreateInfo.pSetLayouts = m_DescriptorSetLayouts.data(); // Optional

		CheckVkResult(vkCreatePipelineLayout(GraphicsContext::GetDevice(), &m_PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

		m_Pipeline = CreateRef<GraphicsPipeline>(context.Device, m_PipelineLayout, context.SwapchainExtent, context.RenderPass);

		for (const auto& [stage, source] : m_Sources)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = (uint32_t)source->Code.size() * sizeof(uint32_t);
			createInfo.pCode = source->Code.data();

			VkShaderModule shaderModule;
			CheckVkResult(vkCreateShaderModule(GraphicsContext::GetDevice(), &createInfo, nullptr, &shaderModule));

			m_Pipeline->AddShaderStage(stage, shaderModule);
			m_Modules[stage] = shaderModule;
		}

		m_Pipeline->VertexInputBindingDescriptions.push_back(m_VertexInputBindingDescription);
		m_Pipeline->VertexInputAttributeDescriptions = m_VertexInputAttributeDescriptions;

		m_Pipeline->PipelineDepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		m_Pipeline->Create();
	}

	void Shader::Bind(VkCommandBuffer commandBuffer)
	{
		m_Pipeline->Bind(commandBuffer);
	}

	void Shader::Reaload()
	{
	}

	void Shader::Reflect()
	{
		for (const auto& [stage, source] : m_Sources)
		{
			spirv_cross::Compiler compiler(source->Code);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			HG_CORE_TRACE("Shader::Reflect - {0} {1}", Utils::ShaderTypeToString(stage), source->FilePath);
			HG_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
			HG_CORE_TRACE("    {0} resources", resources.sampled_images.size());

			HG_CORE_TRACE("Uniform buffers:");
			for (const auto& resource : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32_t bufferSize;
				if (bufferType.basetype == spirv_cross::SPIRType::Struct)
					bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
				else
					bufferSize = (bufferType.width * bufferType.vecsize) / 8;
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				int memberCount = (int)bufferType.member_types.size();
				uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

				HG_CORE_TRACE("  {0}", resource.name);
				HG_CORE_TRACE("    Size = {0}", bufferSize);
				HG_CORE_TRACE("    Binding = {0}", binding);
				HG_CORE_TRACE("    Members = {0}", memberCount);
				HG_CORE_TRACE("    Set = {0}", set);

				if (m_DescriptorSetLayoutBinding[set].size() < binding + 1)
				{
					m_DescriptorSetLayoutBinding[set].resize(binding + 1);
				}

				VkDescriptorSetLayoutBinding& layoutBinding = m_DescriptorSetLayoutBinding[set][binding];

				layoutBinding.binding = binding;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags |= Utils::ShaderTypeToShaderStageFlag(stage);
			}

			HG_CORE_TRACE("Stage Inputs:");
			for (const auto& resource : resources.stage_inputs)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32_t bufferSize;
				if (bufferType.basetype == spirv_cross::SPIRType::Struct)
					bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
				else
					bufferSize = (bufferType.width * bufferType.vecsize) / 8;
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t offset = compiler.get_decoration(resource.id, spv::DecorationAlignment);

				HG_CORE_TRACE("  {0}", resource.name);
				HG_CORE_TRACE("    Size = {0}", bufferSize);
				HG_CORE_TRACE("    Location = {0}", location);
				HG_CORE_TRACE("    Binding = {0}", binding);
				HG_CORE_TRACE("    Offset = {0}", offset);
				HG_CORE_TRACE("    HasDecoration = {0}", compiler.has_decoration(resource.id, spv::DecorationMatrixStride));
			}

			if (stage == ShaderType::Vertex)
			{
				m_VertexInputAttributeDescriptions.resize(resources.stage_inputs.size());

				std::vector<uint32_t> elementSize(resources.stage_inputs.size());

				for (int i = 0; i < resources.stage_inputs.size(); ++i)
				{
					auto id = resources.stage_inputs[i].id;
					auto typeId = resources.stage_inputs[i].base_type_id;
					uint32_t location = compiler.get_decoration(resources.stage_inputs[i].id, spv::DecorationLocation);
					const auto& bufferType = compiler.get_type(typeId);

					uint32_t bufferSize;
					if (bufferType.basetype == spirv_cross::SPIRType::Struct)
						bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
					else
						bufferSize = (bufferType.width * bufferType.vecsize) / 8;

					elementSize[location] = bufferSize;
					m_VertexInputAttributeDescriptions[location].binding = 0;
					m_VertexInputAttributeDescriptions[location].location = location;
					m_VertexInputAttributeDescriptions[location].format = Utils::SpirvBaseTypeToVkFormat(bufferType);
					m_VertexInputAttributeDescriptions[location].offset = 0;
				}

				uint32_t offset = 0;
				for (int i = 0; i < elementSize.size(); ++i)
				{
					m_VertexInputAttributeDescriptions[i].offset = offset;
					offset += elementSize[i];
				}

				m_VertexInputBindingDescription.binding = 0;
				m_VertexInputBindingDescription.stride = offset;
				m_VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			}

			HG_CORE_TRACE("Stage Outputs:");
			for (const auto& resource : resources.stage_outputs)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32_t bufferSize;
				if (bufferType.basetype == spirv_cross::SPIRType::Struct)
					bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
				else
					bufferSize = (bufferType.width * bufferType.vecsize) / 8;
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t offset = compiler.get_decoration(resource.id, spv::DecorationOffset);

				HG_CORE_TRACE("  {0}", resource.name);
				HG_CORE_TRACE("    Size = {0}", bufferSize);
				HG_CORE_TRACE("    Location = {0}", location);
				HG_CORE_TRACE("    Binding = {0}", binding);
				HG_CORE_TRACE("    Offset = {0}", offset);
			}

			HG_CORE_TRACE("Push Constants:");
			for (const auto& resource : resources.push_constant_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32_t bufferSize;
				if (bufferType.basetype == spirv_cross::SPIRType::Struct)
					bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
				else
					bufferSize = (bufferType.width * bufferType.vecsize) / 8;
				uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t offset = compiler.get_decoration(resource.id, spv::DecorationOffset);

				HG_CORE_TRACE("  {0}", resource.name);
				HG_CORE_TRACE("    Size = {0}", bufferSize);
				HG_CORE_TRACE("    Location = {0}", location);
				HG_CORE_TRACE("    Binding = {0}", binding);
				HG_CORE_TRACE("    Offset = {0}", offset);
				HG_CORE_TRACE("    HasOffset = {0}", compiler.has_decoration(resource.id, spv::DecorationOffset));

				VkPushConstantRange pushConstant = {};
				pushConstant.offset = 0;
				pushConstant.size = bufferSize;
				pushConstant.stageFlags |= Utils::ShaderTypeToShaderStageFlag(stage);

				m_PushConstantRanges.push_back(pushConstant);
			}

			HG_CORE_TRACE("Sampled Images:");
			for (const auto& resource : resources.sampled_images)
			{
				VkDescriptorSetLayoutBinding layoutBinding = {};

				const auto& bufferType = compiler.get_type(resource.type_id);
				uint32_t bufferSize;
				if (bufferType.basetype == spirv_cross::SPIRType::Struct)
					bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
				else
					bufferSize = (bufferType.width * bufferType.vecsize) / 8;
				uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
				int memberCount = (int)bufferType.member_types.size();

				HG_CORE_TRACE("  {0}", resource.name);
				HG_CORE_TRACE("    Size = {0}", bufferSize);
				HG_CORE_TRACE("    Binding = {0}", binding);
				HG_CORE_TRACE("    Members = {0}", memberCount);
				HG_CORE_TRACE("    Set = {0}", set);

				layoutBinding.binding = binding;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.descriptorCount = bufferType.array[0];
				layoutBinding.stageFlags |= Utils::ShaderTypeToShaderStageFlag(stage);
				m_DescriptorSetLayoutBinding[set].push_back(layoutBinding);
			}
		}
	}
}
