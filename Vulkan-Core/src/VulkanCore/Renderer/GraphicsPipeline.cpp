#include <vkcpch.h>

#include "GraphicsPipeline.h"

#include "VulkanCore/Utils/RendererUtils.h"

namespace VulkanCore
{
	GraphicsPipeline::GraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass)
		:m_Device(device)
	{
		Viewport.width = (float)swapchainExtent.width;
		Viewport.height = (float)swapchainExtent.height;

		Scissor.extent = swapchainExtent;

		GraphicsPipelineCreateInfo.renderPass = renderPass;
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

		CheckVkResult(vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &Layout));

		GraphicsPipelineCreateInfo.stageCount = (uint32_t)ShaderStageCreateInfos.size();
		GraphicsPipelineCreateInfo.pStages = ShaderStageCreateInfos.data();
		GraphicsPipelineCreateInfo.layout = Layout;
		CheckVkResult(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &PipelineHandle));
		m_Initialized = true;
		return PipelineHandle;
	}

	void GraphicsPipeline::Update(VkExtent2D swapchainExtent, VkRenderPass renderPass)
	{
		Viewport.width = (float)swapchainExtent.width;
		Viewport.height = (float)swapchainExtent.height;

		Scissor.extent = swapchainExtent;

		GraphicsPipelineCreateInfo.renderPass = renderPass;
	}

	void GraphicsPipeline::Destroy()
	{
		vkDestroyPipeline(m_Device, PipelineHandle, nullptr);
		vkDestroyPipelineLayout(m_Device, Layout, nullptr);
		m_Initialized = false;
	}

	void GraphicsPipeline::AddShaderStage(Shader::ShaderType type, VkShaderModule shaderModule, const char* main)
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
