#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

namespace VulkanCore {

	enum class MemoryType
	{
		CPUWritableVertexBuffer,
		GPUOnlyVertexBuffer,
		TransferSourceBuffer,
		CPUWritableIndexBuffer,
		UniformBuffer,
	};

	static VmaMemoryUsage MemoryTypeToVmaMemoryUsage(MemoryType type);

	static VkBufferUsageFlags MemoryTypeToVkBufferUsageFlagBits(MemoryType type);
	
	static VkSharingMode MemoryTypeToVkSharingMode(MemoryType type);

	static bool IsTypeGPUOnly(MemoryType type);

	enum class DataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool,
		Depth32, Depth32Stencil8, Depth24Stencil8, RGBA8, BGRA8
	};

	inline static VkFormat DataTypeToVkFormat(DataType type)
	{
		switch (type)
		{
			case DataType::Float:            return VK_FORMAT_R32_SFLOAT;
			case DataType::Float2:           return VK_FORMAT_R32G32_SFLOAT;
			case DataType::Float3:           return VK_FORMAT_R32G32B32_SFLOAT;
			case DataType::Float4:           return VK_FORMAT_R32G32B32A32_SFLOAT;
			case DataType::Mat3:             return VK_FORMAT_R32G32B32_SFLOAT;
			case DataType::Mat4:             return VK_FORMAT_R32G32B32A32_SFLOAT;
			case DataType::Int:              return VK_FORMAT_R32_SINT;
			case DataType::Int2:             return VK_FORMAT_R32G32_SINT;
			case DataType::Int3:             return VK_FORMAT_R32G32B32_SINT;
			case DataType::Int4:             return VK_FORMAT_R32G32B32A32_SINT;
			case DataType::Bool:             return VK_FORMAT_R8_UINT;
			case DataType::Depth32:          return VK_FORMAT_D32_SFLOAT;
			case DataType::Depth24Stencil8:  return VK_FORMAT_D24_UNORM_S8_UINT;
			case DataType::Depth32Stencil8:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case DataType::BGRA8:            return VK_FORMAT_B8G8R8A8_UNORM;
			case DataType::RGBA8:            return VK_FORMAT_R8G8B8A8_SRGB;
		}

		VKC_CORE_ASSERT(false, "Unknown DataType!");
		return VK_FORMAT_UNDEFINED;
	}

	inline static DataType VkFormatToDataType(VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_R32_SFLOAT:          return DataType::Float;
			case VK_FORMAT_R32G32_SFLOAT:       return DataType::Float2;
			case VK_FORMAT_R32G32B32_SFLOAT:    return DataType::Float3;
			case VK_FORMAT_R32G32B32A32_SFLOAT: return DataType::Float4;
			case VK_FORMAT_R32_SINT:            return DataType::Int;
			case VK_FORMAT_R32G32_SINT:         return DataType::Int2;
			case VK_FORMAT_R32G32B32_SINT:      return DataType::Int3;
			case VK_FORMAT_R32G32B32A32_SINT:   return DataType::Int4;
			case VK_FORMAT_R8_UINT:             return DataType::Bool;
			case VK_FORMAT_D32_SFLOAT:          return DataType::Depth32;
			case VK_FORMAT_D24_UNORM_S8_UINT:   return DataType::Depth24Stencil8;
			case VK_FORMAT_D32_SFLOAT_S8_UINT:  return DataType::Depth32Stencil8;
			case VK_FORMAT_B8G8R8A8_UNORM:      return DataType::BGRA8;
			case VK_FORMAT_R8G8B8A8_SRGB:         return DataType::RGBA8;
		}

		VKC_CORE_ASSERT(false, "Unknown VkFormat!");
		return DataType::None;
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

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec3 Color;

		inline static const BufferLayout Layout = {
			{DataType::Float3, "a_Position"},
			{DataType::Float3, "a_Normal"},
			{DataType::Float2, "a_TexCoords"},
			{DataType::Float3, "a_Color"},
		};
	};

	class Buffer
	{
	public:
		static Ref<Buffer> Create(MemoryType type, uint32_t size);
	public:
		Buffer(MemoryType type, uint32_t size);
		~Buffer();

		void SetData(void* data, uint32_t size);
		void TransferData(uint32_t size, const Ref<Buffer>& src);
		const VkBuffer& GetHandle() const { return m_Handle; }
		uint32_t GetSize() const { return m_Size; }
		MemoryType GetMemoryType() const { return m_Type; }
	private:
		VkBuffer m_Handle;
		VmaAllocation m_Allocation;

		VkBufferCreateInfo m_BufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VmaAllocationCreateInfo m_AllocationCreateInfo = {};

		MemoryType m_Type;
		uint32_t m_Size;
	};

	class VertexBuffer : public Buffer
	{
	public:
		static Ref<VertexBuffer> Create(uint32_t size);
	public:
		VertexBuffer(uint32_t size);
		~VertexBuffer() = default;

		const BufferLayout& GetLayout() const { return m_Layout; }
		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

		VkVertexInputBindingDescription GetInputBindingDescription();
		std::vector<VkVertexInputAttributeDescription> GetInputAttributeDescriptions();
	private:
		BufferLayout m_Layout;
	};
}
