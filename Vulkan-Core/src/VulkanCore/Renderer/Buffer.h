#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

namespace VulkanCore {

	enum class MemoryType
	{
		CPUWritableVertexBuffer,
		CPUWritableIndexBuffer,
		UniformBuffer,
	};

	static VmaMemoryUsage MemoryTypeToVmaMemoryUsage(MemoryType type);

	static VkBufferUsageFlagBits MemoryTypeToVkBufferUsageFlagBits(MemoryType type);
	
	static VkSharingMode MemoryTypeToVkSharingMode(MemoryType type);

	enum class DataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool, Depth
	};

	static VkFormat DataTypeToVkFormat(DataType type)
	{
		switch (type)
		{
			case DataType::Float:    return VK_FORMAT_R32_SFLOAT;
			case DataType::Float2:   return VK_FORMAT_R32G32_SFLOAT;
			case DataType::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;
			case DataType::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
			case DataType::Mat3:     return VK_FORMAT_R32G32B32_SFLOAT;
			case DataType::Mat4:     return VK_FORMAT_R32G32B32A32_SFLOAT;
			case DataType::Int:      return VK_FORMAT_R32_SINT;
			case DataType::Int2:     return VK_FORMAT_R32G32_SINT;
			case DataType::Int3:     return VK_FORMAT_R32G32B32_SINT;
			case DataType::Int4:     return VK_FORMAT_R32G32B32A32_SINT;
			case DataType::Bool:     return VK_FORMAT_R8_UINT;
			case DataType::Depth:     return VK_FORMAT_D32_SFLOAT;
		}

		VKC_CORE_ASSERT(false, "Unknown DataType!");
		return VK_FORMAT_UNDEFINED;
	}

	static uint32_t ShaderDataTypeSize(DataType type)
	{
		switch (type)
		{
			case DataType::Float:    return 4;
			case DataType::Float2:   return 4 * 2;
			case DataType::Float3:   return 4 * 3;
			case DataType::Float4:   return 4 * 4;
			case DataType::Mat3:     return 4 * 3 * 3;
			case DataType::Mat4:     return 4 * 4 * 4;
			case DataType::Int:      return 4;
			case DataType::Int2:     return 4 * 2;
			case DataType::Int3:     return 4 * 3;
			case DataType::Int4:     return 4 * 4;
			case DataType::Bool:     return 1;
		}

		VKC_CORE_ASSERT(false, "Unknown DataType!");
		return 0;
	}

	struct BufferElement
	{
		std::string Name;
		uint32_t Location;
		DataType Type;
		uint32_t Size;
		size_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(DataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
				case DataType::Float:   return 1;
				case DataType::Float2:  return 2;
				case DataType::Float3:  return 3;
				case DataType::Float4:  return 4;
				case DataType::Mat3:    return 3; // 3* float3
				case DataType::Mat4:    return 4; // 4* float4
				case DataType::Int:     return 1;
				case DataType::Int2:    return 2;
				case DataType::Int3:    return 3;
				case DataType::Int4:    return 4;
				case DataType::Bool:    return 1;
			}

			VKC_CORE_ASSERT(false, "Unknown DataType!");
			return 0;
		}
	};

	class BufferLayout
	{
	public:
		BufferLayout() {}

		BufferLayout(std::initializer_list<BufferElement> elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
	private:
		void CalculateOffsetsAndStride()
		{
			size_t offset = 0;
			m_Stride = 0;
			for (size_t i = 0; i < m_Elements.size(); i++)
			{
				if (m_Elements[i].Type == DataType::Mat3)
				{
					auto name = m_Elements[i].Name;
					m_Elements.insert(m_Elements.begin() + i + 1, {DataType::Float3, name, m_Elements[i].Normalized});
					m_Elements.insert(m_Elements.begin() + i + 1, {DataType::Float3, name, m_Elements[i].Normalized});
					m_Elements.insert(m_Elements.begin() + i + 1, {DataType::Float3, name, m_Elements[i].Normalized});
					m_Elements.erase(m_Elements.begin() + i);
				}

				if (m_Elements[i].Type == DataType::Mat4)
				{
					auto name = m_Elements[i].Name;
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float4, name, m_Elements[i].Normalized });

					m_Elements.erase(m_Elements.begin() + i);
				}

				auto& element = m_Elements[i];

				element.Location = (uint32_t)i;
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	struct UniformBufferObject {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Color;

		inline static const BufferLayout Layout = {
			{DataType::Float3, "a_Position"},
			{DataType::Float3, "a_Normal"},
			{DataType::Float3, "a_Color"},
		};
	};

	class MemoryBuffer
	{
	public:
		MemoryBuffer() = default;
		~MemoryBuffer();

		void Create(uint64_t size, MemoryType type);
		void SetData(void* data, size_t size);
		void Destroy();
		
	public:
		VkBuffer Handle;
		VmaAllocation Allocation;

		VkBufferCreateInfo BufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VmaAllocationCreateInfo AllocationCreateInfo = {};

		MemoryType Type;
	private:
		uint64_t m_Size;
		bool m_Initialized = false;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer() = default;
		~VertexBuffer() = default;

		void Create(uint64_t size);
		void Destroy();
		void SetData(void* data, uint64_t size);

		uint64_t GetSize() const { return m_Size; }

		const BufferLayout& GetLayout() const { return m_Layout; }
		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

		VkVertexInputBindingDescription GetInputBindingDescription();
		std::vector<VkVertexInputAttributeDescription> GetInputAttributeDescriptions();

		VkBuffer GetHandle() const { return m_Buffer.Handle; }
	private:
		BufferLayout m_Layout;
		MemoryBuffer m_Buffer;
		uint64_t m_Size;
	};

	class IndexBuffer
	{
	public:
		IndexBuffer() = default;
		~IndexBuffer() = default;

		void Create(uint64_t size);
		void SetData(void* data, uint64_t size);

		VkBuffer GetHandle() const { return m_Buffer.Handle; }
	private:
		MemoryBuffer m_Buffer;
	};
}
