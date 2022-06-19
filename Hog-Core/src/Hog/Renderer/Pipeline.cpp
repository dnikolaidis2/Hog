#include <hgpch.h>

#include "Pipeline.h"

#include <Hog/Utils/RendererUtils.h>
#include <Hog/Renderer/GraphicsContext.h>
#include <Hog/Renderer/Shader.h>

namespace Hog
{
	Ref<Pipeline> Pipeline::CreateGraphics(const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescription,
		VkPipelineLayout layout, VkRenderPass renderPass)
	{
		return CreateRef<GraphicsPipeline>(vertexInputBindingDescription, vertexInputAttributeDescription, layout, renderPass);
	}

	Ref<Pipeline> Pipeline::CreateCompute(VkPipelineLayout layout)
	{
		return CreateRef<ComputePipeline>(layout);
	}

	void Pipeline::AddShaderStage(ShaderType type, VkShaderModule shaderModule, VkSpecializationInfo* specializationInfo, const char* main)
	{
		VkPipelineShaderStageCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = static_cast<VkShaderStageFlagBits>(type.Stage),
			.module = shaderModule,
			.pName = main,
			.pSpecializationInfo = specializationInfo,
		};

		m_ShaderStageCreateInfos.push_back(info);
	}

	void Pipeline::Destroy()
	{
		vkDestroyPipeline(GraphicsContext::GetDevice(), m_Handle, nullptr);
		m_Initialized = false;
	}

	GraphicsPipeline::GraphicsPipeline(const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescription,
		VkPipelineLayout layout, VkRenderPass renderPass)
		:m_PipelineLayout(layout)
	{
		GraphicsPipelineCreateInfo.renderPass = renderPass;
		MultisamplingStateCreateInfo.rasterizationSamples = GraphicsContext::GetMSAASamples();

		VertexInputBindingDescriptions = vertexInputBindingDescription;
		VertexInputAttributeDescriptions = vertexInputAttributeDescription;
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		if (m_Initialized)
			Destroy();
	}

	VkPipeline GraphicsPipeline::Create()
	{
		DynamicStateCreateInfo.dynamicStateCount = (uint32_t)DynamicStates.size();
		DynamicStateCreateInfo.pDynamicStates = DynamicStates.data();

		VertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)VertexInputBindingDescriptions.size();
		VertexInputStateCreateInfo.pVertexBindingDescriptions = VertexInputBindingDescriptions.data(); // Optional
		VertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)VertexInputAttributeDescriptions.size();
		VertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescriptions.data(); // Optional

		GraphicsPipelineCreateInfo.stageCount = (uint32_t)m_ShaderStageCreateInfos.size();
		GraphicsPipelineCreateInfo.pStages = m_ShaderStageCreateInfos.data();
		GraphicsPipelineCreateInfo.layout = m_PipelineLayout;

		std::for_each(ColorBlendAttachmentStates.begin(), ColorBlendAttachmentStates.end(), [&](VkPipelineColorBlendAttachmentState& blendState) { blendState = ColorBlendAttachmentState; });

		ColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(ColorBlendAttachmentStates.size());
		ColorBlendStateCreateInfo.pAttachments = ColorBlendAttachmentStates.data();

		CheckVkResult(vkCreateGraphicsPipelines(GraphicsContext::GetDevice(), VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &m_Handle));

		m_Initialized = true;
		return m_Handle;
	}

	void GraphicsPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handle);
	}

	ComputePipeline::ComputePipeline(VkPipelineLayout layout)
		:m_PipelineLayout(layout)
	{
	}

	ComputePipeline::~ComputePipeline()
	{
		vkDestroyPipeline(GraphicsContext::GetDevice(), m_Handle, nullptr);
		m_Initialized = false;
	}

	VkPipeline ComputePipeline::Create()
	{
		ComputePipelineCreateInfo.stage = m_ShaderStageCreateInfos[0];
		ComputePipelineCreateInfo.layout = m_PipelineLayout;

		CheckVkResult(vkCreateComputePipelines(GraphicsContext::GetDevice(), VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &m_Handle));

		m_Initialized = true;
		return m_Handle;
	}

	void ComputePipeline::Bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Handle);
	}
}
