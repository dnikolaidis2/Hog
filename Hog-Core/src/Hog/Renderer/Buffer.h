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
		~Buffer();

		void WriteData(void* data, uint32_t size, uint64_t bufferOffset = 0, uint64_t dataOffset = 0);
		void ReadData(void* data, uint32_t size, uint64_t bufferOffset = 0, uint64_t dataOffset = 0);
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

	class BufferSubrange
	{
	public:
		static Ref<BufferSubrange> Create(Ref<Buffer> buffer, uint64_t offset, uint32_t size);
	public:
		BufferSubrange(Ref<Buffer> buffer, uint64_t offset, uint32_t size);

		void WriteData(void* data, uint32_t size, uint64_t bufferOffset = 0, uint64_t dataOffset = 0);
		void ReadData(void* data, uint32_t size, uint64_t bufferOffset = 0, uint64_t dataOffset = 0);
	private:
		Ref<Buffer> m_Buffer;
		uint64_t m_Offset;
		uint32_t m_Size;
	};
}
