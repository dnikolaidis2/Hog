#pragma once
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>

namespace VulkanCore {

	class Shader
	{
	public:
		enum class ShaderType { Fragment, Vertex, Compute };
	public:
		static VkShaderStageFlagBits ShaderTypeToVkShaderStageFlagBit(ShaderType type);
	public:
		Shader(const std::string& filepath);
		~Shader();

		VkShaderModule GetVertexShaderModule() const { return m_VertexShaderModule; }
		VkShaderModule GetFragmentShaderModule() const { return m_FragmentShaderModule; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return m_DescriptorSetLayouts; }
		const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescriptions() { return m_VertexInputAttributeDescriptions; }
		VkVertexInputBindingDescription GetVertexInputBindingDescription() const { return m_VertexInputBindingDescription; }
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

		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBinding;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

		VkPipelineLayoutCreateInfo m_PipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};

		VkVertexInputBindingDescription m_VertexInputBindingDescription = {};
		std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributeDescriptions;

		VkPipelineLayout m_PipelineLayout;
	};
}

