#include "vkcpch.h"

#include "Image.h"

#include <stb_image.h>

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

		if (m_Sampler)
			vkDestroySampler(GraphicsContext::GetDevice(), m_Sampler, nullptr);
	}

	void Image::SetData(void* data, uint32_t size)
	{
		auto buffer = Buffer::Create(MemoryType::TransferSourceBuffer, size);
		buffer->SetData(data, size);

		GraphicsContext::ImmediateSubmit([&](VkCommandBuffer commandBuffer)
			{
				VkImageSubresourceRange range = {};
				range.aspectMask = ImageTypeToVkAspectFlag(m_Type);
				range.baseMipLevel = 0;
				range.levelCount = m_LevelCount;
				range.baseArrayLayer = 0;
				range.layerCount = 1;

				VkImageMemoryBarrier imageBarrierToTransfer = {};
				imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

				imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrierToTransfer.image = m_Handle;
				imageBarrierToTransfer.subresourceRange = range;

				imageBarrierToTransfer.srcAccessMask = 0;
				imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				//barrier the image into the transfer-receive layout
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = ImageTypeToVkAspectFlag(m_Type);
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = m_ImageCreateInfo.extent;

				//copy the buffer into the image
				vkCmdCopyBufferToImage(commandBuffer, buffer->GetHandle(), m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;

				imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				//barrier the image into the shader readable layout
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
			});
	}

	VkSampler Image::GetOrCreateSampler()
	{
		if (m_Sampler)
			return m_Sampler;

		//create a sampler for the texture
		VkSamplerCreateInfo samplerInfo = {};

		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		CheckVkResult(vkCreateSampler(GraphicsContext::GetDevice(), &samplerInfo, nullptr, &m_Sampler));

		return m_Sampler;
	}

	void Image::CreateViewForImage()
	{
		m_ViewCreateInfo.image = m_Handle;
		m_ViewCreateInfo.format = m_InternalFormat;
		m_ViewCreateInfo.subresourceRange.aspectMask = ImageTypeToVkAspectFlag(m_Type);
		m_ViewCreateInfo.viewType = ImageTypeToVkImageViewType(m_Type);

		CheckVkResult(vkCreateImageView(GraphicsContext::GetDevice(), &m_ViewCreateInfo, nullptr, &m_View));
	}

	static std::unordered_map<std::string, Ref<Image>> s_Images;

	void TextureLibrary::Add(const std::string& name, const Ref<Image>& image)
	{
		VKC_CORE_ASSERT(!Exists(name), "Image already exists!");
		image->SetGPUIndex((int32_t)s_Images.size());
		s_Images[name] = image;
	}

	Ref<Image> TextureLibrary::LoadFromFile(const std::string& filepath)
	{
		std::filesystem::path path(filepath);
		std::string name;
		if (path.has_filename())
			name = path.filename().string();

		VKC_CORE_ASSERT(!Exists(name), "Image already exists!");

		int width, height, channels;

		stbi_set_flip_vertically_on_load(true);

		stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!pixels) {
			VKC_CORE_ERROR("Failed to load texture file {0}", path);
			return nullptr;
		}

		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
		uint32_t imageSize = width * height * 4;

		auto image = Image::Create(ImageType::Texture, width, height, 1, format);
		
		image->SetData(pixels, imageSize);

		Add(name, image);

		stbi_image_free(pixels);

		return image;
	}

	Ref<Image> TextureLibrary::LoadOrGet(const std::string& filepath)
	{
		std::filesystem::path path(filepath);
		std::string name;
		if (path.has_filename())
			name = path.filename().string();

		if (Exists(name))
			return Get(name);
		else
			return LoadFromFile(filepath);

		return nullptr;
	}

	Ref<Image> TextureLibrary::Get(const std::string& name)
	{
		VKC_CORE_ASSERT(Exists(name), "Image not found!");
		return s_Images[name];
	}

	std::array<Ref<Image>, TEXTURE_ARRAY_SIZE> TextureLibrary::GetLibraryArray()
	{
		std::array<Ref<Image>, TEXTURE_ARRAY_SIZE> arr;

		for (const auto &[name, image]  : s_Images)
		{
			arr[image->GetGPUIndex()] = image;
		}

		return arr;
	}

	void TextureLibrary::Initialize()
	{
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
		uint32_t imageSize = 1 * 1 * 4;
		unsigned char pixels0[] = { 0, 0, 0, 0 };

		auto image = Image::Create(ImageType::Texture, 1, 1, 1, format);

		image->SetData(pixels0, imageSize);

		Add("ones", image);

		format = VK_FORMAT_R8G8B8A8_SRGB;
		imageSize = 1 * 1 * 4;
		unsigned char pixels1[] = { UCHAR_MAX, UCHAR_MAX, UCHAR_MAX, UCHAR_MAX };

		image = Image::Create(ImageType::Texture, 1, 1, 1, format);

		image->SetData(pixels1, imageSize);

		Add("zeros", image);

		format = VK_FORMAT_R8G8B8A8_SRGB;
		imageSize = 1 * 1 * 4;
		unsigned char pixels2[] = { 55, 250, 198, UCHAR_MAX };

		image = Image::Create(ImageType::Texture, 1, 1, 1, format);

		image->SetData(pixels2, imageSize);

		Add("error", image);
	}

	void TextureLibrary::Deinitialize()
	{
		s_Images.clear();
	}

	bool TextureLibrary::Exists(const std::string& name)
	{
		return s_Images.find(name) != s_Images.end();
	}
}
