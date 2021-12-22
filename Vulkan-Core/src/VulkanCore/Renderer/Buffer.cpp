#include "vkcpch.h"

#include "Buffer.h"

#include "GraphicsContext.h"
#include "VulkanCore/Utils/RendererUtils.h"

namespace VulkanCore
{
	VmaMemoryUsage MemoryTypeToVmaMemoryUsage(MemoryType type)
	{
		switch (type)
		{
			case MemoryType::CPUWritableVertexBuffer: return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case MemoryType::CPUWritableIndexBuffer: return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case MemoryType::GPUOnlyVertexBuffer: return VMA_MEMORY_USAGE_GPU_ONLY;
			case MemoryType::TransferSourceBuffer: return VMA_MEMORY_USAGE_CPU_ONLY;
			case MemoryType::UniformBuffer: return VMA_MEMORY_USAGE_CPU_TO_GPU;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VmaMemoryUsage)0;
	}

	VkBufferUsageFlags MemoryTypeToVkBufferUsageFlagBits(MemoryType type)
	{
		switch (type)
		{
			case MemoryType::CPUWritableVertexBuffer: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case MemoryType::CPUWritableIndexBuffer: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case MemoryType::GPUOnlyVertexBuffer: return (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			case MemoryType::TransferSourceBuffer: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			case MemoryType::UniformBuffer: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VkBufferUsageFlags)0;
	}

	VkSharingMode MemoryTypeToVkSharingMode(MemoryType type)
	{
		switch (type)
		{
			case MemoryType::CPUWritableVertexBuffer: return VK_SHARING_MODE_EXCLUSIVE;
			case MemoryType::GPUOnlyVertexBuffer: return VK_SHARING_MODE_EXCLUSIVE;
			case MemoryType::CPUWritableIndexBuffer: return VK_SHARING_MODE_EXCLUSIVE;
			case MemoryType::TransferSourceBuffer: return VK_SHARING_MODE_EXCLUSIVE;
			case MemoryType::UniformBuffer: return VK_SHARING_MODE_EXCLUSIVE;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VkSharingMode)0;
	}

	bool IsTypeGPUOnly(MemoryType type)
	{
		switch (type)
		{
			case MemoryType::GPUOnlyVertexBuffer: return true;
		}

		return false;
	}

	Buffer::Buffer(MemoryType type, uint32_t size)
		:m_Type(type), m_Size(size)
	{
		m_BufferCreateInfo.size = size;
		m_BufferCreateInfo.usage = MemoryTypeToVkBufferUsageFlagBits(type);
		m_BufferCreateInfo.sharingMode = MemoryTypeToVkSharingMode(type);

		m_AllocationCreateInfo.usage = MemoryTypeToVmaMemoryUsage(type);

		//allocate the buffer
		CheckVkResult(vmaCreateBuffer(GraphicsContext::GetAllocator(), &m_BufferCreateInfo, &m_AllocationCreateInfo,
			&m_Handle,
			&m_Allocation,
			nullptr));
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(GraphicsContext::GetAllocator(), m_Handle, m_Allocation);
	}

	Ref<Buffer> Buffer::Create(MemoryType type, uint32_t size)
	{
		return CreateRef<Buffer>(type, size);
	}

	void Buffer::SetData(void* data, uint32_t size)
	{
		VKC_ASSERT(size <= m_Size, "Invalid write command. Tried to write more data then can fit buffer.")

		if (!IsTypeGPUOnly(m_Type))
		{
			void* dstAdr;
			vmaMapMemory(GraphicsContext::GetAllocator(), m_Allocation, &dstAdr);

			memcpy(dstAdr, data, size);

			vmaUnmapMemory(GraphicsContext::GetAllocator(), m_Allocation);
		}
		else
		{
			auto buffer = Buffer::Create(MemoryType::TransferSourceBuffer, size);
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

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		return CreateRef<VertexBuffer>(size);
	}

	VertexBuffer::VertexBuffer(uint32_t size)
		: Buffer(MemoryType::GPUOnlyVertexBuffer, size)
	{
	}

	VkVertexInputBindingDescription VertexBuffer::GetInputBindingDescription()
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
	}
}
