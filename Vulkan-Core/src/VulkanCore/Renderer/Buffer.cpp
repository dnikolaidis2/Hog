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
			case MemoryType::UniformBuffer: return VMA_MEMORY_USAGE_CPU_TO_GPU;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VmaMemoryUsage)0;
	}

	VkBufferUsageFlagBits MemoryTypeToVkBufferUsageFlagBits(MemoryType type)
	{
		switch (type)
		{
			case MemoryType::CPUWritableVertexBuffer: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case MemoryType::CPUWritableIndexBuffer: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case MemoryType::UniformBuffer: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VkBufferUsageFlagBits)0;
	}

	VkSharingMode MemoryTypeToVkSharingMode(MemoryType type)
	{
		switch (type)
		{
			case MemoryType::CPUWritableVertexBuffer: return VK_SHARING_MODE_EXCLUSIVE;
			case MemoryType::CPUWritableIndexBuffer: return VK_SHARING_MODE_EXCLUSIVE;
			case MemoryType::UniformBuffer: return VK_SHARING_MODE_EXCLUSIVE;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VkSharingMode)0;
	}

	MemoryBuffer::~MemoryBuffer()
	{
		if (m_Initialized)
			Destroy();
	}

	void MemoryBuffer::Create(uint64_t size, MemoryType type)
	{
		Type = type;
		m_Size = size;

		BufferCreateInfo.size = size;
		BufferCreateInfo.usage = MemoryTypeToVkBufferUsageFlagBits(type);
		BufferCreateInfo.sharingMode = MemoryTypeToVkSharingMode(type);

		AllocationCreateInfo.usage = MemoryTypeToVmaMemoryUsage(type);

		//allocate the buffer
		CheckVkResult(vmaCreateBuffer(GraphicsContext::GetAllocator(), &BufferCreateInfo, &AllocationCreateInfo,
			&Handle,
			&Allocation,
			nullptr));

		m_Initialized = true;
	}

	void MemoryBuffer::SetData(void* data, size_t size)
	{
		VKC_ASSERT(size <= m_Size, "Invalid write command. Tried to write more data then can fit buffer.")

		void* dstAdr;
		vmaMapMemory(GraphicsContext::GetAllocator(), Allocation, &dstAdr);

		memcpy(dstAdr, data, size);

		vmaUnmapMemory(GraphicsContext::GetAllocator(), Allocation);
	}

	void MemoryBuffer::Destroy()
	{
		vmaDestroyBuffer(GraphicsContext::GetAllocator(), Handle, Allocation);
		m_Initialized = false;
	}

	void VertexBuffer::Create(uint64_t size)
	{
		// Need to move from to gpu only ram
		m_Buffer.Create(size, MemoryType::CPUWritableVertexBuffer);
		m_Size = size;
	}

	void VertexBuffer::Destroy()
	{
		m_Buffer.Destroy();
	}

	void VertexBuffer::SetData(void* data, uint64_t size)
	{
		m_Buffer.SetData(data, size);
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

	void IndexBuffer::Create(uint64_t size)
	{
		// Need to move from to gpu only ram
		m_Buffer.Create(size, MemoryType::CPUWritableIndexBuffer);
	}

	void IndexBuffer::SetData(void* data, uint64_t size)
	{
		m_Buffer.SetData(data, size);
	}
}
