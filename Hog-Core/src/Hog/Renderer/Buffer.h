#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "Hog/Renderer/Types.h"

namespace Hog {

	class Buffer
	{
	public:
		static Ref<Buffer> Create(BufferDescription description, uint32_t size);
	public:
		Buffer(BufferDescription description, uint32_t size);
		virtual ~Buffer();

		void WriteData(void* data, uint32_t size);
		void ReadData(void* data, uint32_t size);
		const VkBuffer& GetHandle() const { return m_Handle; }
		uint32_t GetSize() const { return m_Size; }
		BufferDescription GetBufferDescription() const { return m_Description; }

		operator void* () { return m_AllocationInfo.pMappedData; }
	private:
		VkBuffer m_Handle;
		VmaAllocation m_Allocation;
		VmaAllocationInfo m_AllocationInfo;

		BufferDescription m_Description;
		uint32_t m_Size;
	};

	class VertexBuffer : public Buffer
	{
	public:
		static Ref<VertexBuffer> Create(uint32_t size);
	public:
		VertexBuffer(uint32_t size);
		~VertexBuffer() = default;
	private:
	};
}
