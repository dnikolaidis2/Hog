#pragma once

#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "Constants.h"
#include "vk_mem_alloc.h"

namespace VulkanCore
{
	enum class ImageType
	{
		Depth,
		RenderTarget,
		Texture
	};

	inline static VkImageType ImageTypeToVkImageType(ImageType type)
	{
		switch (type)
		{
			case ImageType::Depth: return VK_IMAGE_TYPE_2D;
			case ImageType::RenderTarget: return VK_IMAGE_TYPE_2D;
			case ImageType::Texture: return VK_IMAGE_TYPE_2D;
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
			case ImageType::Texture: return VK_IMAGE_VIEW_TYPE_2D;
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
			case ImageType::Texture: return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
			case ImageType::Texture: return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown ImageType!");
		return (VkImageAspectFlags)0;
	}

	class Image
	{
	public:
		static Ref<Image> Create(ImageType type, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat& format);
		static Ref<Image> CreateSwapChainImage(VkImage& image, ImageType type, VkFormat& format, VkExtent2D& extent, VkImageViewCreateInfo& viewCreateInfo);
	public:
		Image(ImageType type, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat& format);
		Image(VkImage& image, ImageType type, VkFormat& format, VkExtent2D& extent, VkImageViewCreateInfo& viewCreateInfo);
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
		ImageType m_Type;
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
				.samples = VK_SAMPLE_COUNT_1_BIT,
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

		static std::array<Ref<Image>, TEXTURE_ARRAY_SIZE> GetLibraryArray();

		static void Initialize();
		static void Deinitialize();

		static bool Exists(const std::string& name);
	};
}
