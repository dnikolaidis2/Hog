#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

namespace VulkanCore {

	enum class MemoryType
	{
		GPUOnly, CPUToGPU, CPUOnly
	};

	static VmaMemoryUsage MemoryTypeToVmaMemoryUsage(MemoryType type);

	enum class MemoryUsage
	{
		VertexBuffer, IndexBuffer, UniformBuffer
	};

	VkBufferUsageFlagBits MemoryUsageToVkBufferUsageFlagBits(MemoryUsage usage);

	enum class MemorySharing
	{
		Exclusive, Concurrent
	};

	static VkSharingMode MemorySharingtoVkSharingMode(MemorySharing mode);

	enum class ShaderDataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	static VkFormat ShaderDataTypeToVkFormat(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2:   return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Mat3:     return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Mat4:     return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Int:      return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2:     return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:     return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:     return VK_FORMAT_R32G32B32A32_SINT;
			case ShaderDataType::Bool:     return VK_FORMAT_R8_UINT;
		}

		VKC_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return VK_FORMAT_UNDEFINED;
	}

	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return 4;
			case ShaderDataType::Float2:   return 4 * 2;
			case ShaderDataType::Float3:   return 4 * 3;
			case ShaderDataType::Float4:   return 4 * 4;
			case ShaderDataType::Mat3:     return 4 * 3 * 3;
			case ShaderDataType::Mat4:     return 4 * 4 * 4;
			case ShaderDataType::Int:      return 4;
			case ShaderDataType::Int2:     return 4 * 2;
			case ShaderDataType::Int3:     return 4 * 3;
			case ShaderDataType::Int4:     return 4 * 4;
			case ShaderDataType::Bool:     return 1;
		}

		VKC_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	struct BufferElement
	{
		std::string Name;
		uint32_t Location;
		ShaderDataType Type;
		uint32_t Size;
		size_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
				case ShaderDataType::Float:   return 1;
				case ShaderDataType::Float2:  return 2;
				case ShaderDataType::Float3:  return 3;
				case ShaderDataType::Float4:  return 4;
				case ShaderDataType::Mat3:    return 3; // 3* float3
				case ShaderDataType::Mat4:    return 4; // 4* float4
				case ShaderDataType::Int:     return 1;
				case ShaderDataType::Int2:    return 2;
				case ShaderDataType::Int3:    return 3;
				case ShaderDataType::Int4:    return 4;
				case ShaderDataType::Bool:    return 1;
			}

			VKC_CORE_ASSERT(false, "Unknown ShaderDataType!");
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
				if (m_Elements[i].Type == ShaderDataType::Mat3)
				{
					auto name = m_Elements[i].Name;
					m_Elements.insert(m_Elements.begin() + i + 1, {ShaderDataType::Float3, name, m_Elements[i].Normalized});
					m_Elements.insert(m_Elements.begin() + i + 1, {ShaderDataType::Float3, name, m_Elements[i].Normalized});
					m_Elements.insert(m_Elements.begin() + i + 1, {ShaderDataType::Float3, name, m_Elements[i].Normalized});
					m_Elements.erase(m_Elements.begin() + i);
				}

				if (m_Elements[i].Type == ShaderDataType::Mat4)
				{
					auto name = m_Elements[i].Name;
					m_Elements.insert(m_Elements.begin() + i + 1, { ShaderDataType::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { ShaderDataType::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { ShaderDataType::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { ShaderDataType::Float4, name, m_Elements[i].Normalized });

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
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Color;

		inline static BufferLayout Layout = {
			{ShaderDataType::Float3, "a_Position"},
			{ShaderDataType::Float3, "a_Normal"},
			{ShaderDataType::Float3, "a_Color"},
		};
	};

	class MemoryBuffer
	{
	public:
		MemoryBuffer() = default;
		~MemoryBuffer();

		void Create(uint64_t size, MemoryUsage usage, MemoryType type, MemorySharing mode = MemorySharing::Exclusive);
		void SetData(void* data, size_t size);

		VkBuffer GetBufferObject() const { return m_Buffer; }
		VmaAllocation GetAllocationObject() { return m_Allocation; }

		operator VkBuffer() const { return m_Buffer; }
		operator VmaAllocation() const { return m_Allocation; }
		operator bool() const { return m_Initialized; }
	private:
		bool m_Initialized = false;
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;

		MemoryUsage m_MemoryUsage;
		MemoryType m_MemoryType;
		MemorySharing m_MemorySharing;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer() = default;
		~VertexBuffer() = default;

		void Create(uint64_t size);
		void SetData(void* data, uint64_t size);

		const BufferLayout& GetLayout() const { return m_Layout; }
		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

		VkVertexInputBindingDescription GetInputBindingDescription();
		std::vector<VkVertexInputAttributeDescription> GetInputAttributeDescriptions();
		operator VkBuffer() const { return m_Buffer; }
	private:
		BufferLayout m_Layout;
		MemoryBuffer m_Buffer;
	};

	class IndexBuffer
	{
	public:
		IndexBuffer() = default;
		~IndexBuffer() = default;

		void Create(uint64_t size);
		void SetData(void* data, uint64_t size);

		operator VkBuffer() const { return m_Buffer; }

	private:
		MemoryBuffer m_Buffer;
	};
}
