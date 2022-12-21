#pragma once

#include <shaderc/shaderc.h>
#include <yaml-cpp/yaml.h>
#include <volk.h>

#include "Hog/Renderer/Types.h"

namespace Hog {

	struct ShaderSource
	{
		std::string Name;
		std::filesystem::path FilePath;
		std::filesystem::path CacheFilePath;
		ShaderType Type;
		size_t Hash;
		std::vector<uint32_t> Code;

		ShaderSource() = default;
		ShaderSource(std::string&& name, std::filesystem::path&& filepath, ShaderType type, size_t hash, std::vector<uint32_t>&& code)
			: Name(std::move(name)), FilePath(std::move(filepath)), Type(type), Hash(hash), Code(std::move(code)) {}

		ShaderSource(const ShaderSource& shaderSource) = default;

		static Ref<ShaderSource> Deserialize(YAML::Node& info);
		void Serialize(YAML::Emitter& emitter);

		inline static Ref<ShaderSource> Create(std::string&& name, std::filesystem::path&& filepath, ShaderType type, size_t hash, std::vector<uint32_t>&& code)
		{
			return CreateRef<ShaderSource>(std::forward<std::string>(name), std::forward<std::filesystem::path>(filepath), type, hash, std::forward<std::vector<uint32_t>>(code));
		}
	};

	class ShaderCache
	{
	public:
		static ShaderCache& Get()
		{
			static ShaderCache instance;

			return instance;
		}

		~ShaderCache() { Cleanup(); }
		static void Initialize() { if (Get().m_Initialized == false) Get().InitializeImpl(); }
		static void Cleanup() { if (Get().m_Initialized == true) Get().CleanupImpl(); }
		static Ref<ShaderSource> GetShader(const std::string& name) { return Get().GetShaderImpl(name); }
		static Ref<ShaderSource> ReloadShader(const std::string& name) { return Get().ReloadShaderImpl(name); }
	public:
		ShaderCache(ShaderCache const&) = delete;
		void operator=(ShaderCache const&) = delete;
	private:
		ShaderCache() = default;

		void InitializeImpl();
		void CleanupImpl();
		Ref<ShaderSource> GetShaderImpl(const std::string& name);
		Ref<ShaderSource> ReloadShaderImpl(const std::string& name);
		void SaveToFilesystem();

		static std::vector<uint32_t> CompileShader(const std::string& source, ShaderType type, const std::filesystem::path& filepath);
	private:
		bool m_Initialized = false;

		std::unordered_map<std::string, Ref<ShaderSource>> m_ShaderCache;
	};

	class ShaderReflection
	{
	public:
		struct ReflectionData
		{
			std::array<std::vector<VkDescriptorSetLayoutBinding>, 4> DescriptorSetLayoutBinding;
			std::array<VkDescriptorSetLayout, 4> DescriptorSetLayouts = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

			std::vector<VkVertexInputBindingDescription> VertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;

			std::vector<VkPushConstantRange> PushConstantRanges;

			VkPipelineLayout PipelineLayout;
		};
	public:
		static ReflectionData ReflectPipelineLayout(const std::unordered_map<ShaderType, Ref<ShaderSource>>& sources);
	};
}
