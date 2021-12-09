#include "vkcpch.h"
#include "Shader.h"

#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Utils/RendererUtils.h"
#include "VulkanCore/Renderer/GraphicsContext.h"

#include <fstream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <vulkan/vulkan.h>
#include <yaml-cpp/yaml.h>

static auto& context = VulkanCore::GraphicsContext::Get();

namespace VulkanCore {
	namespace Utils {
		static std::optional<shaderc_shader_kind> ShaderTypeFromString(const std::string& type)
		{
			if (type == "vertex")
				return shaderc_glsl_vertex_shader;
			if (type == "fragment" || type == "pixel")
				return shaderc_glsl_fragment_shader;

			VKC_CORE_ASSERT(false, "Unknown shader type!");
			return std::nullopt;
		}

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "assets/cache/shader/vulkan";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static const char* ShaderStageCachedVulkanFileExtension(shaderc_shader_kind stage)
		{
			switch (stage)
			{
				case shaderc_glsl_vertex_shader:    return ".cached_vulkan.vert";
				case shaderc_glsl_fragment_shader:  return ".cached_vulkan.frag";
			}
			VKC_CORE_ASSERT(false);
			return "";
		}

		static const char* ShaderStageToString(shaderc_shader_kind stage)
		{
			switch (stage)
			{
				case shaderc_glsl_vertex_shader:   return "shaderc_glsl_vertex_shader";
				case shaderc_glsl_fragment_shader: return "shaderc_glsl_fragment_shader";
			}
			VKC_CORE_ASSERT(false);
			return nullptr;
		}

		static shaderc_shader_kind StringToShaderStage(std::string string)
		{
			if (string == "shaderc_glsl_vertex_shader") return shaderc_glsl_vertex_shader;
			if (string == "shaderc_glsl_fragment_shader") return shaderc_glsl_fragment_shader;

			VKC_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}

		static const char* GetShaderCacheDB()
		{
			return "assets/cache/shader/vulkan/.db";
		}

		static std::unordered_map<std::string, std::unordered_map<shaderc_shader_kind, std::size_t>> LoadShaderCacheDB()
		{
			std::unordered_map<std::string, std::unordered_map<shaderc_shader_kind, std::size_t>> db;
			std::filesystem::path path = GetShaderCacheDB();

			if (!std::filesystem::exists(path))
				return db;

			YAML::Node data;
			try
			{
				data = YAML::LoadFile(path.string());
			}
			catch (YAML::ParserException e)
			{
				return db;
			}

			auto shaders = data["Cache Database"];
			if (shaders)
			{
				for (auto shader : shaders)
				{
					std::string name = shader.first.as<std::string>();
					std::unordered_map<shaderc_shader_kind, std::size_t> stageHashes;
					auto stages = data["Cache Database"][name.c_str()];
					if (stages)
					{
						for (auto stage : stages)
						{
							shaderc_shader_kind kind = StringToShaderStage(stage.first.as<std::string>());
							stageHashes.insert({ kind, stage.second.as<std::size_t>()});
						}
					}

					db.insert({ name, stageHashes });
				}
			}

			return db;
		}

		static void SaveShaderCacheDB(std::unordered_map<std::string, std::unordered_map<shaderc_shader_kind, std::size_t>>& db)
		{
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Cache Database" << YAML::Value << YAML::BeginMap;
			for (auto entry : db)
			{
				std::string name = entry.first;
				
				out << YAML::Key << name << YAML::BeginMap;
				for (auto stage : entry.second)
				{
					out << YAML::Key << ShaderStageToString(stage.first) << YAML::Value << stage.second;
				}

				out << YAML::EndMap;
			}

			out << YAML::EndMap;
			out << YAML::EndMap;

			std::ofstream fout(GetShaderCacheDB());
			fout << out.c_str();
		}
	}

	VkShaderStageFlagBits Shader::ShaderTypeToVkShaderStageFlagBit(ShaderType type)
	{
		switch (type)
		{
			case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		return (VkShaderStageFlagBits)0;
	}

	Shader::Shader(const std::string& filepath)
		: m_FilePath(filepath)
	{
		m_Device = context.Device;
		Utils::CreateCacheDirectoryIfNeeded();

		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);

		// Extract name from filepath
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);

		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSources);
			CreateProgram();
			VKC_CORE_WARN("Shader creation took {0} ms", timer.ElapsedMillis());
		}
	}

	Shader::~Shader()
	{
		vkDestroyShaderModule(m_Device, m_VertexShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, m_FragmentShaderModule, nullptr);
	}

	std::string Shader::ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				result.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&result[0], size);
			}
			else
			{
				VKC_CORE_ERROR("Could not read from file '{0}'", filepath);
			}
		}
		else
		{
			VKC_CORE_ERROR("Could not open file '{0}'", filepath);
		}

		return result;
	}

	void Shader::CompileOrGetVulkanBinaries(const std::unordered_map<shaderc_shader_kind, std::string>& shaderSources)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		auto cacheDB = Utils::LoadShaderCacheDB();

		auto& shaderData = m_VulkanSPIRV;
		shaderData.clear();
		for (auto&& [stage, source] : shaderSources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::ShaderStageCachedVulkanFileExtension(stage));

			if (cacheDB.contains(m_Name))
			{
				if (cacheDB[m_Name].contains(stage))
				{
					if (cacheDB[m_Name][stage] == std::hash<std::string>{}(source))
					{
						std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
						if (in.is_open())
						{
							in.seekg(0, std::ios::end);
							auto size = in.tellg();
							in.seekg(0, std::ios::beg);

							auto& data = shaderData[stage];
							data.resize(size / sizeof(uint32_t));
							in.read((char*)data.data(), size);
						}

						// Done lets move on to the next one
						continue;
					}
				}
			}

			// Cache did not contain an up to date version of the shader
			shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, stage, m_FilePath.c_str(), options);
			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				VKC_CORE_ERROR(module.GetErrorMessage());
				VKC_CORE_ASSERT(false);
			}

			shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				auto& data = shaderData[stage];
				out.write((char*)data.data(), data.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}

			std::size_t hash = std::hash<std::string>{}(source);

			if (!cacheDB.contains(m_Name))
			{
				// Never seen this shader before
				cacheDB.insert({ m_Name, {{stage, hash}} });
			}
			else
			{
				if (cacheDB[m_Name].contains(stage))
				{
					// Seen this shader just need to update the cache
					cacheDB[m_Name][stage] = hash;
				}
				else
				{
					// Seen this shader but not his stage
					cacheDB[m_Name].insert({ stage, hash });
				}
			}
		}

		Utils::SaveShaderCacheDB(cacheDB);

		for (auto&& [stage, data] : shaderData)
			Reflect(stage, data);
	}

	void Shader::Reflect(shaderc_shader_kind stage, const std::vector<uint32_t>& shaderData)
	{
		spirv_cross::Compiler compiler(shaderData);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		VKC_CORE_TRACE("OpenGLShader::Reflect - {0} {1}", Utils::ShaderStageToString(stage), m_FilePath);
		VKC_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
		VKC_CORE_TRACE("    {0} resources", resources.sampled_images.size());

		VKC_CORE_TRACE("Uniform buffers:");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize;
			if (bufferType.basetype == spirv_cross::SPIRType::Struct)
				bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType) / 8;
			else
				bufferSize = (bufferType.width * bufferType.vecsize) / 8;
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			int memberCount = (int)bufferType.member_types.size();

			VKC_CORE_TRACE("  {0}", resource.name);
			VKC_CORE_TRACE("    Size = {0}", bufferSize);
			VKC_CORE_TRACE("    Binding = {0}", binding);
			VKC_CORE_TRACE("    Members = {0}", memberCount);
		}

		VKC_CORE_TRACE("Stage Inputs:");
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

			VKC_CORE_TRACE("  {0}", resource.name);
			VKC_CORE_TRACE("    Size = {0}", bufferSize);
			VKC_CORE_TRACE("    Location = {0}", location);
			VKC_CORE_TRACE("    Binding = {0}", binding);
			VKC_CORE_TRACE("    Offset = {0}", offset);
			VKC_CORE_TRACE("    HasDecoration = {0}", compiler.has_decoration(resource.id, spv::DecorationMatrixStride));
		}

		VKC_CORE_TRACE("Stage Outputs:");
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

			VKC_CORE_TRACE("  {0}", resource.name);
			VKC_CORE_TRACE("    Size = {0}", bufferSize);
			VKC_CORE_TRACE("    Location = {0}", location);
			VKC_CORE_TRACE("    Binding = {0}", binding);
			VKC_CORE_TRACE("    Offset = {0}", offset);
		}
	}

	void Shader::CreateProgram()
	{
		m_VertexShaderModule = CreateShaderModule(m_VulkanSPIRV[shaderc_glsl_vertex_shader]);
		m_FragmentShaderModule = CreateShaderModule(m_VulkanSPIRV[shaderc_glsl_fragment_shader]);
	}

	VkShaderModule Shader::CreateShaderModule(const std::vector<uint32_t>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = (uint32_t)code.size() * sizeof(uint32_t);
		createInfo.pCode = code.data();

		VkShaderModule shaderModule;
		CheckVkResult(vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}

	std::unordered_map<shaderc_shader_kind, std::string> Shader::PreProcess(const std::string& source)
	{
		std::unordered_map<shaderc_shader_kind, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0); //Start of shader type declaration line
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos); //End of shader type declaration line
			VKC_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1; //Start of shader type name (after "#type " keyword)
			std::string type = source.substr(begin, eol - begin);
			VKC_CORE_ASSERT(Utils::ShaderTypeFromString(type).has_value(), "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol); //Start of shader code after shader type declaration line
			VKC_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
			pos = source.find(typeToken, nextLinePos); //Start of next shader type declaration line

			shaderSources[Utils::ShaderTypeFromString(type).value()] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}

		return shaderSources;
	}
}
