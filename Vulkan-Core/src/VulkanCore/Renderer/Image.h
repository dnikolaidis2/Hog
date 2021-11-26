#pragma once

#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "vk_mem_alloc.h"

namespace VulkanCore
{
	enum class ImageType
	{
		Depth,
		RenderTarget
	};

	inline static VkImageType ImageTypeToVkImageType(ImageType type)
	{
		switch (type)
		{
			case ImageType::Depth: return VK_IMAGE_TYPE_2D;
			case ImageType::RenderTarget: return VK_IMAGE_TYPE_2D;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageType)0;
	}

	inline static VkImageViewType ImageTypeToVkImageViewType(ImageType type)
	{
		switch (type)
		{
			case ImageType::Depth: return VK_IMAGE_VIEW_TYPE_2D;
			case ImageType::RenderTarget: return VK_IMAGE_VIEW_TYPE_2D;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageViewType)0;
	}

	inline static VkImageUsageFlags ImageTypeToVkImageUsage(ImageType type)
	{
		switch (type)
		{
			case ImageType::Depth: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			case ImageType::RenderTarget: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageUsageFlags)0;
	}

	inline static VkImageAspectFlags ImageTypeToVkAspectFlag(ImageType type)
	{
		switch (type)
		{
			case ImageType::Depth: return VK_IMAGE_ASPECT_DEPTH_BIT;
			case ImageType::RenderTarget: return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageAspectFlags)0;
	}

	class Image
	{
	public:
		VkImage Handle;
		VkImageView View;
		VkFormat InternalFormat = VK_FORMAT_UNDEFINED;
		DataType Format;
		VmaAllocation Allocation;
		ImageType Type;
		uint32_t LevelCount = 1;
		uint32_t Width;
		uint32_t Height;
		bool IsSwapChainImage;
		Image() = default;

		Image(const Image&) = default;
		~Image();

		Image(VkImage image, ImageType type, VkFormat format, VkExtent2D extent)
			: Handle(image), Type(type), InternalFormat(format)
		{
			Format = VkFormatToDataType(format);
			Width = extent.width;
			Height = extent.height;
		}

		void Create();
		void CreateViewForImage();
		void Destroy();

		VkImageCreateInfo ImageCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
		};

		VkImageViewCreateInfo ViewCreateInfo= {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
	private:
		bool m_Allocated = false;
		bool m_Initialized = false;
	};
}
