#include "hgpch.h"

#include "Buffer.h"

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"

namespace Hog
{
	Buffer::Buffer(BufferDescription description, uint32_t size)
		:m_Description(description), m_Size(size)
	{
		VkBufferCreateInfo buffeCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = static_cast<VkBufferUsageFlags>(description),
			.sharingMode = static_cast<VkSharingMode>(description),
		};

		VmaAllocationCreateInfo allocationCreateInfo =
		{
			.flags = description.AllocationCreateFlags,
			.usage = static_cast<VmaMemoryUsage>(description),
		};

		//allocate the buffer
		CheckVkResult(vmaCreateBuffer(GraphicsContext::GetAllocator(), &buffeCreateInfo, &allocationCreateInfo,
			&m_Handle,
			&m_Allocation,
			&m_AllocationInfo));
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(GraphicsContext::GetAllocator(), m_Handle, m_Allocation);
	}

	Ref<Buffer> Buffer::Create(BufferDescription type, uint32_t size)
	{
		return CreateRef<Buffer>(type, size);
	}

	void Buffer::WriteData(void* data, uint32_t size, uint64_t srcOffset, uint64_t dstOffset)
	{
		HG_ASSERT(size <= m_Size, "Invalid write command. Tried to write more data then can fit buffer.");

		VkMemoryPropertyFlags memPropFlags;
		vmaGetAllocationMemoryProperties(GraphicsContext::GetAllocator(), m_Allocation, &memPropFlags);

		if (memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			// Allocation ended up in a mappable memory and is already mapped - write to it directly.

			memcpy((void*)((uint64_t)m_AllocationInfo.pMappedData + dstOffset), (void*)((uint64_t)data + srcOffset), size);

			VkBufferMemoryBarrier2 memoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
				.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = m_Handle,
				.size = m_Size,
			};

			VkDependencyInfo depenedencyInfo = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &memoryBarrier,
			};

			GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer)
			{
				vkCmdPipelineBarrier2(commandBuffer, &depenedencyInfo);
			});
		}
		else
		{
			// Allocation ended up in a non-mappable memory - need to transfer.
			auto stagingBuf = Buffer::Create(BufferDescription::Defaults::TransferSourceBuffer, size);

			// [Executed in runtime]:
			stagingBuf->WriteData(data, size, srcOffset, dstOffset);
			
			//vkCmdPipelineBarrier: VK_ACCESS_HOST_WRITE_BIT --> VK_ACCESS_TRANSFER_READ_BIT

			VkBufferMemoryBarrier2 memoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
				.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = m_Handle,
				.size = m_Size,
			};

			VkDependencyInfo depenedencyInfo = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &memoryBarrier,
			};

			GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer)
			{
				VkBufferCopy copy = {};
				copy.dstOffset = dstOffset;
				copy.srcOffset = srcOffset;
				copy.size = stagingBuf->GetSize();
				vkCmdCopyBuffer(commandBuffer, stagingBuf->GetHandle(), m_Handle, 1, &copy);

				vkCmdPipelineBarrier2(commandBuffer, &depenedencyInfo);
			});
		}
	}

	void Buffer::ReadData(void* data, uint32_t size, uint64_t srcOffset, uint64_t dstOffset)
	{
		HG_ASSERT(size <= m_Size, "Invalid read command. Buffer contents do not fit in data.");
		
		VkMemoryPropertyFlags memPropFlags;
		vmaGetAllocationMemoryProperties(GraphicsContext::GetAllocator(), m_Allocation, &memPropFlags);

		if (memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			// Allocation ended up in a mappable memory and is already mapped - write to it directly.

			VkBufferMemoryBarrier2 memoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
				.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = m_Handle,
				.size = m_Size,
			};

			VkDependencyInfo depenedencyInfo = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &memoryBarrier,
			};

			GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer)
			{
				vkCmdPipelineBarrier2(commandBuffer, &depenedencyInfo);
			});

			memcpy((void*)((uint64_t)data + dstOffset), (void*)((uint64_t)m_AllocationInfo.pMappedData + srcOffset), m_Size);
		}
		else
		{
			// Allocation ended up in a non-mappable memory - need to transfer.
			auto stagingBuf = Buffer::Create(BufferDescription::Defaults::TransferSourceBuffer, size);

			//vkCmdPipelineBarrier: VK_ACCESS_HOST_WRITE_BIT --> VK_ACCESS_TRANSFER_READ_BIT

			VkBufferMemoryBarrier2 memoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = m_Handle,
				.size = m_Size,
			};

			VkDependencyInfo depenedencyInfo = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &memoryBarrier,
			};

			GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer)
			{
				vkCmdPipelineBarrier2(commandBuffer, &depenedencyInfo);

				VkBufferCopy copy = {};
				copy.dstOffset = dstOffset;
				copy.srcOffset = srcOffset;
				copy.size = stagingBuf->GetSize();
				vkCmdCopyBuffer(commandBuffer, m_Handle, stagingBuf->GetHandle(), 1, &copy);
			});

			stagingBuf->ReadData(data, size, srcOffset, dstOffset);
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		return CreateRef<VertexBuffer>(size);
	}

	VertexBuffer::VertexBuffer(uint32_t size)
		: Buffer(BufferDescription::Defaults::GPUOnlyVertexBuffer, size)
	{
	}
}
