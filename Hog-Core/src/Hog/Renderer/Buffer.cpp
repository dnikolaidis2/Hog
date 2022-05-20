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

	void Buffer::SetData(void* data, uint32_t size)
	{
		HG_ASSERT(size <= m_Size, "Invalid write command. Tried to write more data then can fit buffer.")

		if (!m_Description.IsGpuOnly())
		{
			if (m_Description.IsPersistentlyMapped())
			{
				memcpy(m_AllocationInfo.pMappedData, data, size);
			}
			else
			{
				void* dstAdr;
				vmaMapMemory(GraphicsContext::GetAllocator(), m_Allocation, &dstAdr);

				memcpy(dstAdr, data, size);

				vmaUnmapMemory(GraphicsContext::GetAllocator(), m_Allocation);
			}
		}
		else
		{
			auto buffer = Buffer::Create(BufferDescription::Defaults::TransferSourceBuffer, size);
			buffer->SetData(data, size);
			TransferData(size, buffer);
		}
	}

	void Buffer::TransferData(uint32_t size, const Ref<Buffer>& src)
	{
		GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer)
		{
			VkBufferCopy copy = {};
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = src->GetSize();
			vkCmdCopyBuffer(commandBuffer, src->GetHandle(), m_Handle, 1, &copy);
		});
	}

	void Buffer::LockAfterWrite(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage)
	{
		if (m_Description.IsTransferSrc())
		{
			const VkBufferMemoryBarrier2 bufferMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
				.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT,
				.dstStageMask = stage,
				.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = m_Handle,
				.offset = 0,
				.size = VK_WHOLE_SIZE,
			};

			const VkDependencyInfo dependencyInfo = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &bufferMemoryBarrier,
			};

			vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
		}
	}

	void Buffer::LockBeforeRead(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage)
	{
		if (m_Description.IsTransferDst())
		{
			const VkBufferMemoryBarrier2 bufferMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = stage,
				.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = m_Handle,
				.offset = 0,
				.size = VK_WHOLE_SIZE,
			};

			const VkDependencyInfo dependencyInfo = {
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.bufferMemoryBarrierCount = 1,
				.pBufferMemoryBarriers = &bufferMemoryBarrier,
			};

			vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
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

	/*VkVertexInputBindingDescription VertexBuffer::GetInputBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = m_Layout.GetStride();
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> VertexBuffer::GetInputAttributeDescriptions()
	{
		const auto& layout = m_Layout.GetElements();
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(layout.size());
		for (int i = 0; i < layout.size(); ++i)
		{
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = layout[i].Location;
			attributeDescriptions[i].format = DataTypeToVkFormat(layout[i].Type);
			attributeDescriptions[i].offset = (uint32_t)layout[i].Offset;
		}

		return attributeDescriptions;
	}*/
}
