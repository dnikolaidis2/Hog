#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "Hog/Renderer/Constants.h"
#include "Hog/Renderer/Types.h"

namespace Hog
{
	class Image
	{
	public:
		static Ref<Image> Create(ImageDescription description, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		static Ref<Image> Create(ImageDescription description, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		static Ref<Image> Create(ImageDescription description, uint32_t levelCount, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		static Ref<Image> CreateSwapChainImage(VkImage image, ImageDescription description, VkFormat format, VkExtent2D extent, VkImageViewCreateInfo viewCreateInfo);
	public:
		Image(ImageDescription description, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
		Image(VkImage image, ImageDescription description, VkFormat format, VkExtent2D extent, VkImageViewCreateInfo viewCreateInfo);
		~Image();

		void SetData(void* data, uint32_t size);

		VkImageView GetImageView() const { return m_View; }
		VkFormat GetInternalFormat() const { return m_InternalFormat; }
		VkSampler GetOrCreateSampler();
		void SetGPUIndex(int32_t ind) { m_GPUIndex = ind; }
		int32_t GetGPUIndex() const { return m_GPUIndex; }
	private:
		void CreateViewForImage();
	private:
		VkImage m_Handle;
		VkImageView m_View;
		VkFormat m_InternalFormat = VK_FORMAT_UNDEFINED;
		DataType m_Format;
		VmaAllocation m_Allocation;
		ImageDescription m_Description;
		uint32_t m_LevelCount = 1;
		uint32_t m_Width;
		uint32_t m_Height;
		bool m_IsSwapChainImage;
		bool m_Allocated;
		int32_t m_GPUIndex  = 0;

		VkDescriptorSet m_DescriptorSet = nullptr;
		VkSampler m_Sampler = nullptr;

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

	class TextureLibrary
	{
	public:
		TextureLibrary() = delete;

		static void Add(const std::string& name, const Ref<Image>& image);
		static Ref<Image> LoadFromFile(const std::string& filepath);
		static Ref<Image> LoadOrGet(const std::string& filepath);

		static Ref<Image> Get(const std::string& name);

		static std::vector<Ref<Image>> GetLibraryArray();

		static void Initialize();
		static void Deinitialize();

		static bool Exists(const std::string& name);
	};
}
