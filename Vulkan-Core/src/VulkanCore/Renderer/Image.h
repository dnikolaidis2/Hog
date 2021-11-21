#pragma once

#include <vulkan/vulkan.h>

namespace VulkanCore
{
	enum class TextureFormat
	{
		FORMAT_RGBA8,
		FORMAT_DEPTH
	};

	enum class TextureType
	{
		TYPE_2D,
		TYPE_CUBIC
	};

	struct Image
	{
		VkImage VulkanImage;
		VkImageView View;
		VkFormat InternalFormat;
		struct Options
		{
			TextureType Type = TextureType::TYPE_2D;
			TextureFormat Format = TextureFormat::FORMAT_RGBA8;
			uint32_t LevelCount = 1;
			uint32_t Width;
			uint32_t Height;
		} Options;

		bool IsSwapChainImage;

		Image() = default;

		Image(const Image&) = default;

		Image(VkImage image, VkImageView imageView, VkFormat format, VkExtent2D extent)
			: VulkanImage(image), View(imageView), InternalFormat(format)
		{
			Options.Width = extent.width;
			Options.Height = extent.height;
		}
	};
}
