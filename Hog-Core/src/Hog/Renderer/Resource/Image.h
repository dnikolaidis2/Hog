#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include "Hog/Renderer/Types.h"

namespace Hog
{
	class Image
	{
	public:
		static Ref<Image> LoadFromFile(const std::string& filepath);
		static Ref<Image> Create(ImageDescription description, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		static Ref<Image> Create(ImageDescription description, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		static Ref<Image> Create(ImageDescription description, uint32_t levelCount, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		static Ref<Image> CreateSwapChainImage(VkImage image, ImageDescription description, VkFormat format, VkExtent2D extent, VkImageViewCreateInfo viewCreateInfo);
	public:
		Image(ImageDescription description, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		Image(VkImage image, ImageDescription description, VkFormat format, VkExtent2D extent, VkImageViewCreateInfo viewCreateInfo);
		~Image();

		void SetData(void* data, uint32_t size);

		void SetImageLayout(VkImageLayout layout) { m_Description.ImageLayout = layout; }
		void ExecuteBarrier(VkCommandBuffer commandBuffer, const BarrierDescription& description);

		VkImageView GetImageView() const { return m_View; }
		VkFormat GetFormat() const { return m_Description.Format; }
		const ImageDescription& GetDescription() const {return m_Description;}
		VkSampleCountFlagBits GetSamples() const { return m_Samples; }
		VkExtent2D GetExtent() const { return VkExtent2D(m_Width, m_Height); }
		VkImageLayout GetImageLayout() const { return m_Description.ImageLayout; }
		uint32_t GetLevelCount() const { return m_LevelCount; }
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
	private:
		void CreateViewForImage();
	private:
		VkImage m_Handle;
		VkImageView m_View;
		VkFormat m_InternalFormat = VK_FORMAT_UNDEFINED;
		VkSampleCountFlagBits m_Samples = VK_SAMPLE_COUNT_1_BIT;
		DataType m_Format;
		VmaAllocation m_Allocation;
		ImageDescription m_Description;
		uint32_t m_LevelCount = 1;
		uint32_t m_Width;
		uint32_t m_Height;
		bool m_IsSwapChainImage;
		bool m_Allocated;

		VkImageCreateInfo m_ImageCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.mipLevels = 1,
				.arrayLayers = 1,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
		};

		VkImageViewCreateInfo m_ViewCreateInfo = {
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
	};
}
