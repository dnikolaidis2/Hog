#include "vkcpch.h"

#include "Image.h"

#include "VulkanCore/Renderer/GraphicsContext.h"
#include "VulkanCore/Utils/RendererUtils.h"

namespace VulkanCore
{
	Image::~Image()
	{
		if (m_Initialized)
		{
			Destroy();
		}
	}

	void Image::Create()
	{
		if (InternalFormat != VK_FORMAT_UNDEFINED)
			Format = VkFormatToDataType(InternalFormat);

		ImageCreateInfo.extent = { Width, Height, 1 };
		ImageCreateInfo.imageType = ImageTypeToVkImageType(Type);
		ImageCreateInfo.format = InternalFormat;
		ImageCreateInfo.usage = ImageTypeToVkImageUsage(Type);

		//for the depth image, we want to allocate it from GPU local memory
		VmaAllocationCreateInfo imageAllocationInfo = {};
		imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		CheckVkResult(vmaCreateImage(GraphicsContext::GetAllocator(), &ImageCreateInfo,
			&imageAllocationInfo, &Handle, &Allocation, nullptr));

		CreateViewForImage();

		m_Initialized = true;
		m_Allocated = true;
	}

	void Image::CreateViewForImage()
	{
		ViewCreateInfo.image = Handle;
		ViewCreateInfo.format = InternalFormat;
		ViewCreateInfo.subresourceRange.aspectMask = ImageTypeToVkAspectFlag(Type);
		ViewCreateInfo.viewType = ImageTypeToVkImageViewType(Type);

		CheckVkResult(vkCreateImageView(GraphicsContext::GetDevice(), &ViewCreateInfo, nullptr, &View));

		m_Initialized = true;
	}

	void Image::Destroy()
	{
		vkDestroyImageView(GraphicsContext::GetDevice(), View, nullptr);
		if (m_Allocated)
			vmaDestroyImage(GraphicsContext::GetAllocator(), Handle, Allocation);

		m_Initialized = false;
	}
}
