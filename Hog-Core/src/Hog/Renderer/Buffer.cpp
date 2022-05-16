#include "hgpch.h"

#include "Buffer.h"

#include "GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"

namespace Hog
{
	VmaMemoryUsage BufferTypeToVmaMemoryUsage(BufferType type)
	{
		switch (type)
		{
			case BufferType::CPUWritableVertexBuffer:	return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case BufferType::CPUWritableIndexBuffer:	return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case BufferType::GPUOnlyVertexBuffer:		return VMA_MEMORY_USAGE_GPU_ONLY;
			case BufferType::TransferSourceBuffer:		return VMA_MEMORY_USAGE_CPU_ONLY;
			case BufferType::UniformBuffer:				return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case BufferType::ReadbackUniformBuffer:		return VMA_MEMORY_USAGE_AUTO;
		}

		HG_CORE_ASSERT(false, "Unknown BufferType!");
		return (VmaMemoryUsage)0;
	}

	VmaAllocationCreateFlags BufferTypeToVmaAllocationCreateFlags(BufferType type)
	{
		switch (type)
		{
			case BufferType::CPUWritableVertexBuffer:	return 0;
			case BufferType::CPUWritableIndexBuffer:	return 0;
			case BufferType::GPUOnlyVertexBuffer:		return 0;
			case BufferType::TransferSourceBuffer:		return 0;
			case BufferType::UniformBuffer:				return 0;
			case BufferType::ReadbackUniformBuffer:		return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		HG_CORE_ASSERT(false, "Unknown BufferType!");
		return (VmaAllocationCreateFlags)0;
	}

	VkBufferUsageFlags BufferTypeToVkBufferUsageFlags(BufferType type)
	{
		switch (type)
		{
			case BufferType::CPUWritableVertexBuffer:	return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case BufferType::CPUWritableIndexBuffer:	return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case BufferType::GPUOnlyVertexBuffer:		return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			case BufferType::TransferSourceBuffer:		return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			case BufferType::UniformBuffer:				return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			case BufferType::ReadbackUniformBuffer:		return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		HG_CORE_ASSERT(false, "Unknown BufferType!");
		return (VkBufferUsageFlags)0;
	}

	VkSharingMode BufferTypeToVkSharingMode(BufferType type)
	{
		switch (type)
		{
			case BufferType::CPUWritableVertexBuffer:	return VK_SHARING_MODE_EXCLUSIVE;
			case BufferType::GPUOnlyVertexBuffer:		return VK_SHARING_MODE_EXCLUSIVE;
			case BufferType::CPUWritableIndexBuffer:	return VK_SHARING_MODE_EXCLUSIVE;
			case BufferType::TransferSourceBuffer:		return VK_SHARING_MODE_EXCLUSIVE;
			case BufferType::UniformBuffer:				return VK_SHARING_MODE_EXCLUSIVE;
			case BufferType::ReadbackUniformBuffer:		return VK_SHARING_MODE_EXCLUSIVE;
		}

		HG_CORE_ASSERT(false, "Unknown BufferType!");
		return (VkSharingMode)0;
	}

	bool IsPersistentlyMapped(BufferType type)
	{
		return (bool)(BufferTypeToVmaAllocationCreateFlags(type) & VMA_ALLOCATION_CREATE_MAPPED_BIT);
	}

	bool IsTypeGPUOnly(BufferType type)
	{
		switch (type)
		{
			case BufferType::GPUOnlyVertexBuffer: return true;
		}

		return false;
	}

	Buffer::Buffer(BufferType type, uint32_t size)
		:m_Type(type), m_Size(size)
	{
		m_BufferCreateInfo.size = size;
		m_BufferCreateInfo.usage = BufferTypeToVkBufferUsageFlags(type);
		m_BufferCreateInfo.sharingMode = BufferTypeToVkSharingMode(type);

		m_AllocationCreateInfo.usage = BufferTypeToVmaMemoryUsage(type);
		m_AllocationCreateInfo.flags = BufferTypeToVmaAllocationCreateFlags(type);

		//allocate the buffer
		CheckVkResult(vmaCreateBuffer(GraphicsContext::GetAllocator(), &m_BufferCreateInfo, &m_AllocationCreateInfo,
			&m_Handle,
			&m_Allocation,
			&m_AllocationInfo));
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(GraphicsContext::GetAllocator(), m_Handle, m_Allocation);
	}

	Ref<Buffer> Buffer::Create(BufferType type, uint32_t size)
	{
		return CreateRef<Buffer>(type, size);
	}

	void Buffer::SetData(void* data, uint32_t size)
	{
		HG_ASSERT(size <= m_Size, "Invalid write command. Tried to write more data then can fit buffer.")

		if (!IsTypeGPUOnly(m_Type))
		{
			if (IsPersistentlyMapped(m_Type))
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
			auto buffer = Buffer::Create(BufferType::TransferSourceBuffer, size);
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

	void Buffer::LockAfterWrite(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage)
	{
		if (BufferTypeToVkBufferUsageFlags(m_Type) & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
		{
			const VkBufferMemoryBarrier2 bufferMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_HOST_BIT,
				.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
				.dstStageMask = stage,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
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

	void Buffer::LockBeforeRead(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage)
	{
		if (BufferTypeToVkBufferUsageFlags(m_Type) & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
		{
			const VkBufferMemoryBarrier2 bufferMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = stage,
				.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
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
		: Buffer(BufferType::GPUOnlyVertexBuffer, size)
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
