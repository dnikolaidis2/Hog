#include <hgpch.h>

#include "Pipeline.h"

#include <Hog/Utils/RendererUtils.h>
#include <Hog/Renderer/GraphicsContext.h>
#include <Hog/Renderer/Shader.h>

namespace Hog
{
	GraphicsPipeline::GraphicsPipeline(VkDevice device, VkPipelineLayout layout, VkExtent2D swapchainExtent, VkRenderPass renderPass)
		:m_Device(device), m_PipelineLayout(layout)
	{
		Viewport.width = (float)swapchainExtent.width;
		Viewport.height = (float)swapchainExtent.height;

		Scissor.extent = swapchainExtent;

		GraphicsPipelineCreateInfo.renderPass = renderPass;
		MultisamplingStateCreateInfo.rasterizationSamples = GraphicsContext::GetMSAASamples();
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

		GraphicsPipelineCreateInfo.stageCount = (uint32_t)ShaderStageCreateInfos.size();
		GraphicsPipelineCreateInfo.pStages = ShaderStageCreateInfos.data();
		GraphicsPipelineCreateInfo.layout = m_PipelineLayout;

		CheckVkResult(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &PipelineHandle));

		m_Initialized = true;
		return PipelineHandle;
	}

	void GraphicsPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION();

		VkExtent2D extent = GraphicsContext::GetExtent();
		Viewport.width = (float)extent.width;
		Viewport.height = (float)extent.height;

		Scissor.extent = extent;

		vkCmdSetViewport(commandBuffer, 0, 1, &Viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &Scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineHandle);
	}

	void GraphicsPipeline::Destroy()
	{
		vkDestroyPipeline(m_Device, PipelineHandle, nullptr);
		m_Initialized = false;
	}

	void GraphicsPipeline::AddShaderStage(ShaderType type, VkShaderModule shaderModule, const char* main)
	{
		VkPipelineShaderStageCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = Shader::ShaderTypeToVkShaderStageFlagBit(type),
			.module = shaderModule,
			.pName = main,
		};

		ShaderStageCreateInfos.push_back(info);
	}
}
