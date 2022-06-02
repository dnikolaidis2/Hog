#pragma once

#include <volk.h>

#include "Hog/Renderer/Types.h"

namespace Hog
{
	class Pipeline
	{
	public:
		static Ref<Pipeline> CreateGraphics(const std::vector <VkVertexInputBindingDescription>& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescription,
			VkPipelineLayout layout, VkRenderPass renderPass);
		static Ref<Pipeline> CreateCompute(VkPipelineLayout layout);

		virtual ~Pipeline() = default;

		virtual VkPipeline Create() = 0;
		virtual void Bind(VkCommandBuffer commandBuffer) = 0;

		void AddShaderStage(ShaderType type, VkShaderModule shaderModule, VkSpecializationInfo* specializationInfo, const char* main = "main");
		void Destroy();
		operator VkPipeline() const { return m_Handle; }
	protected:
		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageCreateInfos;
		VkPipeline m_Handle = VK_NULL_HANDLE;
		bool m_Initialized = false;
	};

	class GraphicsPipeline : public Pipeline
	{
	public:
		GraphicsPipeline(const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescription, VkPipelineLayout layout, VkRenderPass renderPass);
		~GraphicsPipeline() override;

		virtual VkPipeline Create() override;
		virtual void Bind(VkCommandBuffer commandBuffer) override;
	public:
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
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineMultisampleStateCreateInfo MultisamplingStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.sampleShadingEnable = VK_TRUE,
			.minSampleShading = .2f, // Optional
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
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
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
	private:
		VkPipelineLayout m_PipelineLayout;
	};

	class ComputePipeline : public Pipeline
	{
	public:
		ComputePipeline(VkPipelineLayout layout);
		~ComputePipeline() override;

		virtual VkPipeline Create() override;
		virtual void Bind(VkCommandBuffer commandBuffer) override;
	public:
		VkComputePipelineCreateInfo ComputePipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		};
	private:
		VkPipelineLayout m_PipelineLayout;
	};
}
