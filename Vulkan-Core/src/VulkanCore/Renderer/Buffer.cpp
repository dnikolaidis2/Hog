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
			case MemoryType::GPUOnly: return VMA_MEMORY_USAGE_GPU_ONLY;
			case MemoryType::CPUToGPU: return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case MemoryType::CPUOnly: return VMA_MEMORY_USAGE_CPU_ONLY;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryType!");
		return (VmaMemoryUsage)0;
	}

	VkBufferUsageFlagBits MemoryUsageToVkBufferUsageFlagBits(MemoryUsage usage)
	{
		switch (usage)
		{
			case MemoryUsage::IndexBuffer: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case MemoryUsage::VertexBuffer: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case MemoryUsage::UniformBuffer: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}

		VKC_CORE_ASSERT(false, "Unknown MemoryUsage!");
		return (VkBufferUsageFlagBits)0;
	}

	VkSharingMode MemorySharingtoVkSharingMode(MemorySharing mode)
	{
		switch (mode)
		{
			case MemorySharing::Exclusive: return VK_SHARING_MODE_EXCLUSIVE;
			case MemorySharing::Concurrent: return VK_SHARING_MODE_CONCURRENT;
		}

		VKC_CORE_ASSERT(false, "Unknown MemorySharing!");
		return (VkSharingMode)0;
	}

	MemoryBuffer::~MemoryBuffer()
	{
		vmaDestroyBuffer(GraphicsContext::GetAllocator(), m_Buffer, m_Allocation);
	}

	void MemoryBuffer::Create(uint64_t size, MemoryUsage usage, MemoryType type, MemorySharing mode)
	{
		m_MemoryUsage = usage;
		m_MemoryType = type;
		m_MemorySharing = mode;

		//allocate vertex buffer
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//this is the total size, in bytes, of the buffer we are allocating
		bufferInfo.size = size;
		//this buffer is going to be used as a Vertex Buffer
		bufferInfo.usage = MemoryUsageToVkBufferUsageFlagBits(usage);
		bufferInfo.sharingMode = MemorySharingtoVkSharingMode(mode);


		//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = MemoryTypeToVmaMemoryUsage(type);

		//allocate the buffer
		CheckVkResult(vmaCreateBuffer(GraphicsContext::GetAllocator(), &bufferInfo, &vmaallocInfo,
			&m_Buffer,
			&m_Allocation,
			nullptr));

		m_Initialized = true;
	}

	void MemoryBuffer::SetData(void* data, size_t size)
	{
		void* dstAdr;
		vmaMapMemory(GraphicsContext::GetAllocator(), m_Allocation, &dstAdr);

		memcpy(dstAdr, data, size);

		vmaUnmapMemory(GraphicsContext::GetAllocator(), m_Allocation);
	}

	void VertexBuffer::Create(uint64_t size)
	{
		// Need to move from to gpu only ram
		m_Buffer.Create(size, MemoryUsage::VertexBuffer, MemoryType::CPUToGPU, MemorySharing::Exclusive);
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
			attributeDescriptions[i].format = ShaderDataTypeToVkFormat(layout[i].Type);
			attributeDescriptions[i].offset = (uint32_t)layout[i].Offset;
		}

		return attributeDescriptions;
	}

	void IndexBuffer::Create(uint64_t size)
	{
		// Need to move from to gpu only ram
		m_Buffer.Create(size, MemoryUsage::IndexBuffer, MemoryType::CPUToGPU, MemorySharing::Exclusive);
	}

	void IndexBuffer::SetData(void* data, uint64_t size)
	{
		m_Buffer.SetData(data, size);
	}
}
