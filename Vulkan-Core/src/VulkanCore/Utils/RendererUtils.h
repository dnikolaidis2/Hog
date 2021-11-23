#pragma once
#include "VulkanCore/Core/Assert.h"
#include "VulkanCore/Core/Log.h"

#include <vulkan/vulkan.h>

namespace VulkanCore
{
	static void CheckVkResult(VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			VKC_CORE_ERROR("Vulkan check failed with code: {0}", result);
			VKC_CORE_ASSERT(false)
		}
	}
}
