#pragma once
#include "Hog/Core/Assert.h"
#include "Hog/Core/Log.h"

#include <vulkan/vulkan.h>

namespace Hog
{
	inline static void CheckVkResult(VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			HG_CORE_ERROR("Vulkan check failed with code: {0}", result);
			HG_CORE_ASSERT(false)
		}
	}
}
