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
#include <spirv_reflect.h>
#include <vulkan/vulkan.h>
#include <yaml-cpp/yaml.h>

#include "Constants.h"

AutoCVar_String CVar_ShaderCacheDBFile("shader.cacheDBFile", "Shader cache database filename", ".db", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderCacheDir("shader.cachePath", "Shader cache directory", "assets/cache/shader/vulkan", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderSourceDir("shader.sourceDir", "Shader source directory", "assets/shaders/", CVarFlags::EditReadOnly);
AutoCVar_String CVar_ShaderCompiledFileExtension("shader.compiledFileExtension", "Shader compiled file extension", ".spv", CVarFlags::EditReadOnly);
AutoCVar_Int	CVar_ShaderOptimizationLevel("shader.optimizationLevel",
	"Shader compilation optimization level. 0 zero optimization, 1 optimize for size, 2 optimize for performance",
	0, CVarFlags::None);

namespace Hog {
	namespace Utils {
		// Returns the size in bytes of the provided VkFormat.
		// As this is only intended for vertex attribute formats, not all VkFormats are supported.
		static uint32_t FormatSize(VkFormat format)
		{
			uint32_t result = 0;
			switch (format) {
			case VK_FORMAT_UNDEFINED: result = 0; break;
			case VK_FORMAT_R4G4_UNORM_PACK8: result = 1; break;
			case VK_FORMAT_R4G4B4A4_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_R5G6B5_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_B5G6R5_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_R5G5B5A1_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_B5G5R5A1_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_R8_UNORM: result = 1; break;
			case VK_FORMAT_R8_SNORM: result = 1; break;
			case VK_FORMAT_R8_USCALED: result = 1; break;
			case VK_FORMAT_R8_SSCALED: result = 1; break;
			case VK_FORMAT_R8_UINT: result = 1; break;
			case VK_FORMAT_R8_SINT: result = 1; break;
			case VK_FORMAT_R8_SRGB: result = 1; break;
			case VK_FORMAT_R8G8_UNORM: result = 2; break;
			case VK_FORMAT_R8G8_SNORM: result = 2; break;
			case VK_FORMAT_R8G8_USCALED: result = 2; break;
			case VK_FORMAT_R8G8_SSCALED: result = 2; break;
			case VK_FORMAT_R8G8_UINT: result = 2; break;
			case VK_FORMAT_R8G8_SINT: result = 2; break;
			case VK_FORMAT_R8G8_SRGB: result = 2; break;
			case VK_FORMAT_R8G8B8_UNORM: result = 3; break;
			case VK_FORMAT_R8G8B8_SNORM: result = 3; break;
			case VK_FORMAT_R8G8B8_USCALED: result = 3; break;
			case VK_FORMAT_R8G8B8_SSCALED: result = 3; break;
			case VK_FORMAT_R8G8B8_UINT: result = 3; break;
			case VK_FORMAT_R8G8B8_SINT: result = 3; break;
			case VK_FORMAT_R8G8B8_SRGB: result = 3; break;
			case VK_FORMAT_B8G8R8_UNORM: result = 3; break;
			case VK_FORMAT_B8G8R8_SNORM: result = 3; break;
			case VK_FORMAT_B8G8R8_USCALED: result = 3; break;
			case VK_FORMAT_B8G8R8_SSCALED: result = 3; break;
			case VK_FORMAT_B8G8R8_UINT: result = 3; break;
			case VK_FORMAT_B8G8R8_SINT: result = 3; break;
			case VK_FORMAT_B8G8R8_SRGB: result = 3; break;
			case VK_FORMAT_R8G8B8A8_UNORM: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SNORM: result = 4; break;
			case VK_FORMAT_R8G8B8A8_USCALED: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SSCALED: result = 4; break;
			case VK_FORMAT_R8G8B8A8_UINT: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SINT: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SRGB: result = 4; break;
			case VK_FORMAT_B8G8R8A8_UNORM: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SNORM: result = 4; break;
			case VK_FORMAT_B8G8R8A8_USCALED: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SSCALED: result = 4; break;
			case VK_FORMAT_B8G8R8A8_UINT: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SINT: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SRGB: result = 4; break;
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_USCALED_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_UINT_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SINT_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_SNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_USCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_UINT_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_SINT_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_SNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_USCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_UINT_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_SINT_PACK32: result = 4; break;
			case VK_FORMAT_R16_UNORM: result = 2; break;
			case VK_FORMAT_R16_SNORM: result = 2; break;
			case VK_FORMAT_R16_USCALED: result = 2; break;
			case VK_FORMAT_R16_SSCALED: result = 2; break;
			case VK_FORMAT_R16_UINT: result = 2; break;
			case VK_FORMAT_R16_SINT: result = 2; break;
			case VK_FORMAT_R16_SFLOAT: result = 2; break;
			case VK_FORMAT_R16G16_UNORM: result = 4; break;
			case VK_FORMAT_R16G16_SNORM: result = 4; break;
			case VK_FORMAT_R16G16_USCALED: result = 4; break;
			case VK_FORMAT_R16G16_SSCALED: result = 4; break;
			case VK_FORMAT_R16G16_UINT: result = 4; break;
			case VK_FORMAT_R16G16_SINT: result = 4; break;
			case VK_FORMAT_R16G16_SFLOAT: result = 4; break;
			case VK_FORMAT_R16G16B16_UNORM: result = 6; break;
			case VK_FORMAT_R16G16B16_SNORM: result = 6; break;
			case VK_FORMAT_R16G16B16_USCALED: result = 6; break;
			case VK_FORMAT_R16G16B16_SSCALED: result = 6; break;
			case VK_FORMAT_R16G16B16_UINT: result = 6; break;
			case VK_FORMAT_R16G16B16_SINT: result = 6; break;
			case VK_FORMAT_R16G16B16_SFLOAT: result = 6; break;
			case VK_FORMAT_R16G16B16A16_UNORM: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SNORM: result = 8; break;
			case VK_FORMAT_R16G16B16A16_USCALED: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SSCALED: result = 8; break;
			case VK_FORMAT_R16G16B16A16_UINT: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SINT: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SFLOAT: result = 8; break;
			case VK_FORMAT_R32_UINT: result = 4; break;
			case VK_FORMAT_R32_SINT: result = 4; break;
			case VK_FORMAT_R32_SFLOAT: result = 4; break;
			case VK_FORMAT_R32G32_UINT: result = 8; break;
			case VK_FORMAT_R32G32_SINT: result = 8; break;
			case VK_FORMAT_R32G32_SFLOAT: result = 8; break;
			case VK_FORMAT_R32G32B32_UINT: result = 12; break;
			case VK_FORMAT_R32G32B32_SINT: result = 12; break;
			case VK_FORMAT_R32G32B32_SFLOAT: result = 12; break;
			case VK_FORMAT_R32G32B32A32_UINT: result = 16; break;
			case VK_FORMAT_R32G32B32A32_SINT: result = 16; break;
			case VK_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
			case VK_FORMAT_R64_UINT: result = 8; break;
			case VK_FORMAT_R64_SINT: result = 8; break;
			case VK_FORMAT_R64_SFLOAT: result = 8; break;
			case VK_FORMAT_R64G64_UINT: result = 16; break;
			case VK_FORMAT_R64G64_SINT: result = 16; break;
			case VK_FORMAT_R64G64_SFLOAT: result = 16; break;
			case VK_FORMAT_R64G64B64_UINT: result = 24; break;
			case VK_FORMAT_R64G64B64_SINT: result = 24; break;
			case VK_FORMAT_R64G64B64_SFLOAT: result = 24; break;
			case VK_FORMAT_R64G64B64A64_UINT: result = 32; break;
			case VK_FORMAT_R64G64B64A64_SINT: result = 32; break;
			case VK_FORMAT_R64G64B64A64_SFLOAT: result = 32; break;
			case VK_FORMAT_B10G11R11_UFLOAT_PACK32: result = 4; break;
			case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: result = 4; break;

			default:
				break;
			}
			return result;
		}

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
			switch (kind)
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
				pushConstant.stageFlags |= Utils::ShaderTypeToShaderStageFlag(stage);

				m_PushConstantRanges.push_back(pushConstant);
			}

			if (stage == ShaderType::Vertex)
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
					uint32_t format_size = Utils::FormatSize(attribute.format);
					attribute.offset = bindingDescription.stride;
					bindingDescription.stride += format_size;
				}
				// Nothing further is done with attribute_descriptions or binding_description
				// in this sample. A real application would probably derive this information from its
				// mesh format(s); a similar mechanism could be used to ensure mesh/shader compatibility.
				m_VertexInputBindingDescriptions.push_back(bindingDescription);
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
