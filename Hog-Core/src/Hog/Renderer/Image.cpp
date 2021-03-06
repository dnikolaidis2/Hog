#include "hgpch.h"

#include "Image.h"

#include <stb_image.h>

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Buffer.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Core/CVars.h"

namespace Hog
{
	Ref<Image> Image::LoadFromFile(const std::string& filepath)
	{
		std::filesystem::path path(filepath);
		std::string name;
		if (path.has_filename())
			name = path.filename().string();

		int width, height, channels;

		stbi_set_flip_vertically_on_load(false);

		stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		uint32_t imageSize = width * height * 4;

		uint32_t mipLevels;

		if (*CVarSystem::Get()->GetIntCVar("renderer.enableMipMapping"))
		{
			mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		}
		else
		{
			mipLevels = 1;
		}

		Ref<Image> image = Image::Create(ImageDescription::Defaults::Texture, width, height, mipLevels, format);

		image->SetData(pixels, imageSize);

		stbi_image_free(pixels);

		return image;
	}

	Ref<Image> Image::Create(ImageDescription description, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples)
	{
		return CreateRef<Image>(description, width, height, levelCount, format, samples);
	}

	Ref<Image> Image::Create(ImageDescription description, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples)
	{
		VkExtent2D extent = GraphicsContext::GetExtent();
		return CreateRef<Image>(description, extent.width, extent.height, levelCount, format, samples);
	}

	Ref<Image> Image::Create(ImageDescription description, uint32_t levelCount, VkSampleCountFlagBits samples)
	{
		VkExtent2D extent = GraphicsContext::GetExtent();
		return CreateRef<Image>(description, extent.width, extent.height, levelCount, description, samples);
	}

	Ref<Image> Image::CreateSwapChainImage(VkImage image, ImageDescription type, VkFormat format, VkExtent2D extent,
	                                       VkImageViewCreateInfo viewCreateInfo)
	{
		return CreateRef<Image>(image, type, format, extent, viewCreateInfo);
	}

	Image::Image(ImageDescription description, uint32_t width, uint32_t height, uint32_t levelCount, VkFormat format, VkSampleCountFlagBits samples)
		: m_InternalFormat(format), m_Description(description), m_Format(m_InternalFormat), m_Width(width), m_Height(height), m_Allocated(true), m_LevelCount(levelCount), m_Samples(samples)
	{
		m_ImageCreateInfo.extent = { m_Width, m_Height, 1 };
		m_ImageCreateInfo.imageType = static_cast<VkImageType>(m_Description);
		m_ImageCreateInfo.format = m_InternalFormat;
		m_ImageCreateInfo.usage = static_cast<VkImageUsageFlags>(m_Description);
		m_ImageCreateInfo.samples = m_Samples;
		m_ImageCreateInfo.mipLevels = m_LevelCount;

		//for the depth image, we want to allocate it from GPU local memory
		VmaAllocationCreateInfo imageAllocationInfo = {};
		imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		CheckVkResult(vmaCreateImage(GraphicsContext::GetAllocator(), &m_ImageCreateInfo,
			&imageAllocationInfo, &m_Handle, &m_Allocation, nullptr));

		CreateViewForImage();
	}

	Image::Image(VkImage image, ImageDescription type, VkFormat format, VkExtent2D extent,
		VkImageViewCreateInfo viewCreateInfo)
		: m_Handle(image), m_InternalFormat(format), m_Format(format), m_Description(type)
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

	void Image::SetData(void* data, uint32_t size)
	{
		auto buffer = Buffer::Create(BufferDescription::Defaults::TransferSourceBuffer, size);
		buffer->WriteData(data, size);

		GraphicsContext::ImmediateSubmit([&](VkCommandBuffer commandBuffer)
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.image = m_Handle;
			barrier.subresourceRange.aspectMask = m_Description.ImageAspectFlags;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = m_LevelCount;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			//barrier the image into the transfer-receive layout
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = m_Description.ImageAspectFlags;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = m_ImageCreateInfo.extent;

			//copy the buffer into the image
			vkCmdCopyBufferToImage(commandBuffer, buffer->GetHandle(), m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			//mip map generation

			int32_t mipWidth = m_Width;
			int32_t mipHeight = m_Height;

			for (uint32_t i = 1; i < m_LevelCount; i++) {
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(commandBuffer,
					m_Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = m_LevelCount - 1;

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			//barrier the image into the shader readable layout
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);
		});

		m_Description.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void Image::ExecuteBarrier(VkCommandBuffer commandBuffer, const BarrierDescription& description)
	{
		VkImageMemoryBarrier2 memoryBarrier =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = static_cast<VkPipelineStageFlags2>(description.SrcStage),
			.srcAccessMask = static_cast<VkAccessFlags2>(description.SrcAccessMask),
			.dstStageMask = static_cast<VkPipelineStageFlags2>(description.DstStage),
			.dstAccessMask = static_cast<VkAccessFlags2>(description.DstAccessMask),
			.oldLayout = static_cast<VkImageLayout>(description.OldLayout),
			.newLayout = static_cast<VkImageLayout>(description.NewLayout),
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = m_Handle,
			.subresourceRange = {
				.aspectMask = m_Description.ImageAspectFlags,
				.baseMipLevel = 0,
				.levelCount = m_LevelCount,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};

		VkDependencyInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &memoryBarrier,
		};

		vkCmdPipelineBarrier2(commandBuffer, &info);
		
		m_Description.ImageLayout = memoryBarrier.newLayout;
	}

	void Image::CreateViewForImage()
	{
		m_ViewCreateInfo.image = m_Handle;
		m_ViewCreateInfo.format = m_InternalFormat;
		m_ViewCreateInfo.subresourceRange.aspectMask = m_Description.ImageAspectFlags;
		m_ViewCreateInfo.viewType = static_cast<VkImageViewType>(m_Description);
		m_ViewCreateInfo.subresourceRange.levelCount = m_LevelCount;

		CheckVkResult(vkCreateImageView(GraphicsContext::GetDevice(), &m_ViewCreateInfo, nullptr, &m_View));
	}
}
