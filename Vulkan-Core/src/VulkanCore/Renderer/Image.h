#pragma once

#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "vk_mem_alloc.h"

namespace VulkanCore
{
	enum class ImageType
	{
		RGBA8,
		Depth
	};

	inline static VkImageType ImageTypeToVkImageType(ImageType type)
	{
		switch (type)
		{
		case ImageType::Depth: return VK_IMAGE_TYPE_2D;
		case ImageType::RGBA8: return VK_IMAGE_TYPE_2D;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageType)0;
	}

	inline static VkImageViewType ImageTypeToVkImageViewType(ImageType type)
	{
		switch (type)
		{
		case ImageType::Depth: return VK_IMAGE_VIEW_TYPE_2D;
		case ImageType::RGBA8: return VK_IMAGE_VIEW_TYPE_2D;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageViewType)0;
	}

	inline static VkImageUsageFlags ImageTypeToVkImageUsage(ImageType type)
	{
		switch (type)
		{
		case ImageType::Depth: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		case ImageType::RGBA8: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageUsageFlags)0;
	}

	inline static VkImageAspectFlags ImageTypeToVkAspectFlag(ImageType type)
	{
		switch (type)
		{
		case ImageType::Depth: return VK_IMAGE_ASPECT_DEPTH_BIT;
		case ImageType::RGBA8: return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageAspectFlags)0;
	}

	class Image
	{
	public:
		VkImage Handle;
		VkImageView View;
		VkFormat InternalFormat;
		DataType Format;
		VmaAllocation Allocation;
		ImageType Type = ImageType::RGBA8;
		uint32_t LevelCount = 1;
		uint32_t Width;
		uint32_t Height;
		bool IsSwapChainImage;
		bool Allocated;

		Image() = default;

		Image(const Image&) = default;

		Image(VkImage image, VkImageView imageView, VkFormat format, VkExtent2D extent)
			: Handle(image), View(imageView), InternalFormat(format)
		{
			Width = extent.width;
			Height = extent.height;
		}

		void Create();
		VkImageCreateInfo info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
		};

		VkImageViewCreateInfo viewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.subresourceRange = {
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
	};
}
