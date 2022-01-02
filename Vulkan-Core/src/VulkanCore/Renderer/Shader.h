#pragma once
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>

#include <VulkanCore/Renderer/GraphicsPipeline.h>

namespace VulkanCore {

	enum class ShaderType { Fragment, Vertex, Compute };

	class Shader
	{
	public:
		static VkShaderStageFlagBits ShaderTypeToVkShaderStageFlagBit(ShaderType type);
		static Ref<Shader> Create(const std::string& filepath);
	public:
		Shader(const std::string& filepath);
		~Shader();

		Ref<GraphicsPipeline> CreateOrGetDefaultPipeline();

		VkShaderModule GetVertexShaderModule() const { return m_VertexShaderModule; }
		VkShaderModule GetFragmentShaderModule() const { return m_FragmentShaderModule; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		const std::array<VkDescriptorSetLayout, 4>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescriptions() { return m_VertexInputAttributeDescriptions; }
		VkVertexInputBindingDescription GetVertexInputBindingDescription() const { return m_VertexInputBindingDescription; }
		const std::string& GetName() const { return m_Name; }
	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<shaderc_shader_kind, std::string> PreProcess(const std::string& source);

		void CompileOrGetVulkanBinaries(const std::unordered_map<shaderc_shader_kind, std::string>& shaderSources);

		void Reflect(shaderc_shader_kind stage, const std::vector<uint32_t>& shaderData);
		void CreateProgram();
		VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code);
		void GeneratePipelineLayout();
	private:
		VkDevice m_Device;

		std::string m_Name;
		std::string m_FilePath;

		std::unordered_map<shaderc_shader_kind, std::vector<uint32_t>> m_VulkanSPIRV;

		VkShaderModule m_VertexShaderModule;
		VkShaderModule m_FragmentShaderModule;

		std::array<std::vector<VkDescriptorSetLayoutBinding>, 4> m_DescriptorSetLayoutBinding;
		std::array<VkDescriptorSetLayout, 4> m_DescriptorSetLayouts;

		std::vector<VkPushConstantRange> m_PushConstantRanges;

		VkPipelineLayoutCreateInfo m_PipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};

		VkVertexInputBindingDescription m_VertexInputBindingDescription = {};
		std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributeDescriptions;

		VkPipelineLayout m_PipelineLayout;
		Ref<GraphicsPipeline> m_DefaultPipeline;
	};

	class ShaderLibrary
	{
	public:
		ShaderLibrary() = delete;

		static void Add(const std::string& name, const Ref<Shader>& shader);
		static void Add(const Ref<Shader>& shader);
		static void LoadDirectory(const std::string& directory);
		static Ref<Shader> Load(const std::string& filepath);
		static Ref<Shader> Load(const std::string& name, const std::string& filepath);
		static Ref<Shader> LoadOrGet(const std::string& filepath);

		static Ref<Shader> Get(const std::string& name);

		static void Deinitialize();

		static bool Exists(const std::string& name);
	};
}

