#include "vkcpch.h"

#include "Material.h"
#include <VulkanCore/Renderer/GraphicsContext.h>

namespace VulkanCore
{
	static auto& context = GraphicsContext::Get();

	Ref<Material> Material::Create(Ref<Shader> shader, Ref<GraphicsPipeline> pipeline)
	{
		Ref<Shader> shaderRef;
		Ref<GraphicsPipeline> pipelineRef;

		if (shader)
			shaderRef = shader;
		else
			shaderRef = CreateRef<Shader>("assets/shaders/Basic.glsl");

		if (pipeline)
			pipelineRef = pipeline;
		else
		{
			pipelineRef = CreateRef<GraphicsPipeline>(context.Device, shaderRef->GetPipelineLayout(), context.SwapchainExtent, context.RenderPass);

			pipelineRef->AddShaderStage(Shader::ShaderType::Vertex, shaderRef->GetVertexShaderModule());
			pipelineRef->AddShaderStage(Shader::ShaderType::Fragment, shaderRef->GetFragmentShaderModule());

			pipelineRef->VertexInputBindingDescriptions.push_back(shaderRef->GetVertexInputBindingDescription());
			pipelineRef->VertexInputAttributeDescriptions = shaderRef->GetVertexInputAttributeDescriptions();

			pipelineRef->PipelineDepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

			pipelineRef->Create();
		}

		return CreateRef<Material>(shaderRef, pipelineRef);
	}

	void Material::Bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr)
	{
		VKC_PROFILE_FUNCTION()
	}
}
