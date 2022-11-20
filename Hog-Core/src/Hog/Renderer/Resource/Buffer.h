#pragma once

#include <glm/glm.hpp>

#include <volk.h>

#include "vk_mem_alloc.h"

#include "Hog/Renderer/Types.h"

namespace Hog {

	class Buffer
	{
	public:
		static Ref<Buffer> Create(BufferDescription description, size_t size);
	public:
		Buffer(BufferDescription description, size_t size);
		~Buffer();

		void WriteData(void* data, size_t size, size_t bufferOffset = 0, size_t dataOffset = 0);
		void ReadData(void* data, size_t size, size_t bufferOffset = 0, size_t dataOffset = 0);
		const VkBuffer& GetHandle() const { return m_Handle; }
		size_t GetSize() const { return m_Size; }
		BufferDescription GetBufferDescription() const { return m_Description; }

		VkDeviceAddress GetBufferDeviceAddress();

		operator void* () { return m_AllocationInfo.pMappedData; }
	private:
		VkBuffer m_Handle;
		VmaAllocation m_Allocation;
		VmaAllocationInfo m_AllocationInfo;

		BufferDescription m_Description;
		size_t m_Size;
	};

	class BufferRegion
	{
	public:
		static Ref<BufferRegion> Create(Ref<Buffer> buffer, size_t offset, size_t size);
	public:
		BufferRegion(Ref<Buffer> buffer, size_t offset, size_t size);

		void WriteData(void* data, size_t size, size_t bufferOffset = 0, size_t dataOffset = 0);
		void ReadData(void* data, size_t size, size_t bufferOffset = 0, size_t dataOffset = 0);
		size_t GetSize() const { return m_Size; }
		size_t GetOffset() const { return m_Offset; }
	private:
		Ref<Buffer> m_Buffer;
		size_t m_Offset;
		size_t m_Size;
	};
}
