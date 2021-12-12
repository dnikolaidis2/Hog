#pragma once

#include <VulkanCore/Renderer/RendererObject.h>
#include <VulkanCore/Core/Base.h>

namespace VulkanCore
{
	class Renderer
	{
	public:
		static void Reset();
		static void DrawObject(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr, const Ref<RendererObject> object);
		static void DrawObjects(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr, const std::vector<Ref<RendererObject>>& objects);
	};
}
