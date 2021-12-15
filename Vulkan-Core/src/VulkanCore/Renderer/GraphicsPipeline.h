#pragma once

#include <vulkan/vulkan.h>

namespace VulkanCore
{
	enum class ShaderType;

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(VkDevice device, VkPipelineLayout layout, VkExtent2D swapchainExtent, VkRenderPass renderPass);
		~GraphicsPipeline();

		VkPipeline Create();
		void Bind(VkCommandBuffer commandBuffer);
		void Destroy();
		void AddShaderStage(ShaderType type, VkShaderModule shaderModule, const char* main = "main");

		operator VkPipeline() const { return PipelineHandle; }
	private:
	public:
		std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos;

		std::vector<VkVertexInputBindingDescription> VertexInputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;

		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		};

		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		VkViewport Viewport = {
			.x = 0.0f,
			.y = 0.0f,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D Scissor = {
			.offset = {0, 0}
		};

		VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &Viewport,
			.scissorCount = 1,
			.pScissors = &Scissor,
		};

		VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};

		VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
			.colorBlendOp = VK_BLEND_OP_ADD, // Optional
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
			.alphaBlendOp = VK_BLEND_OP_ADD, // Optional
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineMultisampleStateCreateInfo MultisamplingStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f, // Optional
			.pSampleMask = nullptr, // Optional
			.alphaToCoverageEnable = VK_FALSE, // Optional
			.alphaToOneEnable = VK_FALSE, // Optional
		};

		VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &ColorBlendAttachmentState,
			.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
		};

		std::vector<VkDynamicState> DynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
			// VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_ALWAYS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.minDepthBounds = 0.0f, // Optional
			.maxDepthBounds = 1.0f, // Optional
		};

		VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			// .dynamicStateCount = DynamicStates.size(),
			// .pDynamicStates = DynamicStates.data(),
		};

		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			// .stageCount = 2,
			// .pStages = shaderStages,
			.pVertexInputState = &VertexInputStateCreateInfo,
			.pInputAssemblyState = &InputAssemblyStateCreateInfo,
			.pViewportState = &ViewportStateCreateInfo,
			.pRasterizationState = &RasterizationStateCreateInfo,
			.pMultisampleState = &MultisamplingStateCreateInfo,
			.pDepthStencilState = &PipelineDepthStencilCreateInfo, // Optional
			.pColorBlendState = &ColorBlendStateCreateInfo,
			.pDynamicState = &DynamicStateCreateInfo, // Optional
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE, // Optional
			.basePipelineIndex = -1, // Optional
		};

		VkPipeline PipelineHandle;
	private:
		VkDevice m_Device;
		VkPipelineLayout m_PipelineLayout;
		bool m_Initialized = false;
	};
}
