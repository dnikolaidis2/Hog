#include "vkcpch.h"
#include "Renderer.h"

#include <VulkanCore/Renderer/GraphicsPipeline.h>

namespace VulkanCore
{
	struct RendererState
	{
		Ref<GraphicsPipeline> BoundPipeline = nullptr;
	};

	static RendererState s_Data;

	void Renderer::Reset()
	{
		s_Data.BoundPipeline = nullptr;
	}

	void Renderer::DrawObject(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr, const Ref<RendererObject> object)
	{
		VKC_PROFILE_FUNCTION();

		Ref<Material> mat = object->GetMaterial();
		Ref<Shader> shader = mat->GetShader();
		Ref<GraphicsPipeline> pipeline = mat->GetPipeline();

		if (pipeline != s_Data.BoundPipeline)
		{
			pipeline->Bind(commandBuffer);
			s_Data.BoundPipeline = pipeline;
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(), 0, 1, descriptorSetPtr, 0, nullptr);

		object->Draw(commandBuffer);
	}

	void Renderer::DrawObjects(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr, const std::vector<Ref<RendererObject>>& objects)
	{
		VKC_PROFILE_FUNCTION();
		for (const auto& obj : objects)
		{
			DrawObject(commandBuffer, descriptorSetPtr, obj);
		}
	}
}
