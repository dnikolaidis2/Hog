#include "vkcpch.h"

#include "Image.h"

#include "VulkanCore/Renderer/GraphicsContext.h"
#include "VulkanCore/Utils/RendererUtils.h"

namespace VulkanCore
{
	Ref<Image> Image::Create(ImageType type, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat& format)
	{
		return CreateRef<Image>(type, width, height, levelCount, format);
	}

	Ref<Image> Image::CreateSwapChainImage(VkImage& image, ImageType type, VkFormat& format, VkExtent2D& extent,
	                                       VkImageViewCreateInfo& viewCreateInfo)
	{
		return CreateRef<Image>(image, type, format, extent, viewCreateInfo);
	}

	Image::Image(ImageType type, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat& format)
		: m_InternalFormat(format), m_Type(type), m_Format(VkFormatToDataType(m_InternalFormat)), m_Width(width), m_Height(height), m_Allocated(true)
	{
		m_ImageCreateInfo.extent = { width, height, 1 };
		m_ImageCreateInfo.imageType = ImageTypeToVkImageType(type);
		m_ImageCreateInfo.format = m_InternalFormat;
		m_ImageCreateInfo.usage = ImageTypeToVkImageUsage(m_Type);

		//for the depth image, we want to allocate it from GPU local memory
		VmaAllocationCreateInfo imageAllocationInfo = {};
		imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		CheckVkResult(vmaCreateImage(GraphicsContext::GetAllocator(), &m_ImageCreateInfo,
			&imageAllocationInfo, &m_Handle, &m_Allocation, nullptr));

		CreateViewForImage();
	}

	Image::Image(VkImage& image, ImageType type, VkFormat& format, VkExtent2D& extent,
		VkImageViewCreateInfo& viewCreateInfo)
		: m_Handle(image), m_InternalFormat(format), m_Format(VkFormatToDataType(format)), m_Type(type)
		, m_Width(extent.width), m_Height(extent.height), m_Allocated(false), m_ViewCreateInfo(viewCreateInfo)
	{
		CreateViewForImage();
	}

	Image::~Image()
	{
		vkDestroyImageView(GraphicsContext::GetDevice(), m_View, nullptr);
		if (m_Allocated)
			vmaDestroyImage(GraphicsContext::GetAllocator(), m_Handle, m_Allocation);
	}

	void Image::CreateViewForImage()
	{
		m_ViewCreateInfo.image = m_Handle;
		m_ViewCreateInfo.format = m_InternalFormat;
		m_ViewCreateInfo.subresourceRange.aspectMask = ImageTypeToVkAspectFlag(m_Type);
		m_ViewCreateInfo.viewType = ImageTypeToVkImageViewType(m_Type);

		CheckVkResult(vkCreateImageView(GraphicsContext::GetDevice(), &m_ViewCreateInfo, nullptr, &m_View));
	}
}
