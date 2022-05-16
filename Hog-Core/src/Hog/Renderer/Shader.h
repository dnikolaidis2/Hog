#pragma once

#include <shaderc/shaderc.h>
#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan.h>

#include "Hog/Renderer/Pipeline.h"

namespace Hog {

	enum class ShaderType { Fragment, Vertex, Compute, AnyHit, RayGeneration, Intersection, Miss, ClosestHit, Mesh };

	namespace Utils {

		VkShaderStageFlagBits ShaderTypeToShaderStageFlag(ShaderType type);

	};

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

		~ShaderCache() { Deinitialize(); }
		static void Initialize() { if (Get().m_Initialized == false) Get().InitializeImpl(); }
		static void Deinitialize() { if (Get().m_Initialized == true) Get().DeinitializeImpl(); }
		static Ref<ShaderSource> GetShader(const std::string& name) { return Get().GetShaderImpl(name); }
		static Ref<ShaderSource> ReloadShader(const std::string& name) { return Get().ReloadShaderImpl(name); }
	public:
		ShaderCache(ShaderCache const&) = delete;
		void operator=(ShaderCache const&) = delete;
	private:
		ShaderCache() = default;

		void InitializeImpl();
		void DeinitializeImpl();
		Ref<ShaderSource> GetShaderImpl(const std::string& name);
		Ref<ShaderSource> ReloadShaderImpl(const std::string& name);
		void SaveToFilesystem();

		static std::vector<uint32_t> CompileShader(const std::string& source, ShaderType type, const std::filesystem::path& filepath);
	private:
		bool m_Initialized = false;

		std::unordered_map<std::string, Ref<ShaderSource>> m_ShaderCache;
	};

	class Shader
	{
	public:
		static Ref<Shader> Create(const std::string& name, const std::string& vertex, const std::string& fragment);
		static Ref<Shader> Create(const std::string& name);
	public:
		Shader(const std::string& name)
			: m_Name(name) {}
		~Shader();

		void AddStage(Ref<ShaderSource> source);
		void AddStage(const std::string& name);
		void Generate(VkSpecializationInfo specializationInfo);
		void Generate(VkRenderPass, VkSpecializationInfo specializationInfo);
		void Bind(VkCommandBuffer commandBuffer);
		void Reaload();

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		/*VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		const std::array<VkDescriptorSetLayout, 4>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescriptions() { return m_VertexInputAttributeDescriptions; }
		VkVertexInputBindingDescription GetVertexInputBindingDescription() const { return m_VertexInputBindingDescription; }*/
		
	private:
		void Reflect();
	private:
		std::string m_Name;
		std::unordered_map<ShaderType, Ref<ShaderSource>> m_Sources;
		std::unordered_map<ShaderType, VkShaderModule> m_Modules;

		std::array<std::vector<VkDescriptorSetLayoutBinding>, 4> m_DescriptorSetLayoutBinding;
		std::array<VkDescriptorSetLayout, 4> m_DescriptorSetLayouts;

		VkVertexInputBindingDescription m_VertexInputBindingDescription = {};
		std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributeDescriptions;

		std::vector<VkPushConstantRange> m_PushConstantRanges;

		VkPipelineLayoutCreateInfo m_PipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};

		VkPipelineLayout m_PipelineLayout;
		Ref<Pipeline> m_Pipeline;
	};
}
