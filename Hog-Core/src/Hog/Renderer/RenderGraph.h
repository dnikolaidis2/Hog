#pragma once

#include "Hog/Renderer/Shader.h"
#include "Hog/Renderer/Buffer.h"

namespace Hog
{
	struct VertexElement
	{
		std::string Name;
		uint32_t Location;
		DataType Type;
		uint32_t Size;
		size_t Offset;
		bool Normalized;

		VertexElement() = default;

		VertexElement(DataType type, const std::string& name, bool normalized = false)
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

			HG_CORE_ASSERT(false, "Unknown DataType!");
			return 0;
		}
	};

	class VertexInputLayout
	{
	public:
		VertexInputLayout() {}

		VertexInputLayout(std::initializer_list<VertexElement> elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<VertexElement>& GetElements() const { return m_Elements; }

		std::vector<VertexElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<VertexElement>::iterator end() { return m_Elements.end(); }
		std::vector<VertexElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<VertexElement>::const_iterator end() const { return m_Elements.end(); }
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
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float3, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float3, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Float3, name, m_Elements[i].Normalized });
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
		std::vector<VertexElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	enum class ResourceBindLocation
	{
		Compute, Vertex, Fragment, Combined,
	};

	enum class ResourceType
	{
		Uniform, Constant, PushConstant, Storage, Sampler, SamplerArray
	};

	struct ResourceElement
	{
		std::string Name;
		ResourceType Type;
		ResourceBindLocation BindLocation;
		Ref<Buffer> Buffer = nullptr;
		uint32_t ConstantID = 0;
		size_t ConstantSize = 0;
		void* ConstantDataPointer = nullptr;
		uint32_t Binding = 0;
		uint32_t Set = 0;

		ResourceElement(const std::string& name, ResourceType type, ResourceBindLocation bindLocation, Ref<Hog::Buffer> buffer, uint32_t binding, uint32_t set)
			: Name(name), Type(type), BindLocation(bindLocation), Buffer(buffer), Binding(binding), Set(set) {}

		ResourceElement(const std::string& name, ResourceType type, ResourceBindLocation bindLocation, uint32_t constantID, size_t constantSize, void* dataPointer)
			: Name(name), Type(type), BindLocation(bindLocation), ConstantID(constantID), ConstantSize(constantSize), ConstantDataPointer(dataPointer) {}

		ResourceElement(const std::string& name, ResourceType type, ResourceBindLocation bindLocation, size_t constantSize, void* dataPointer)
			: Name(name), Type(type), BindLocation(bindLocation), ConstantSize(constantSize), ConstantDataPointer(dataPointer) {}

		ResourceElement() = default;
	};

	class ResourceLayout
	{
	public:
		ResourceLayout() = default;

		ResourceLayout(std::initializer_list<ResourceElement> elements)
			: m_Elements(elements)	{}

		const std::vector<ResourceElement>& GetElements() const { return m_Elements; }

		bool ContainsType(ResourceType type) const;

		std::vector<ResourceElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<ResourceElement>::iterator end() { return m_Elements.end(); }
		std::vector<ResourceElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<ResourceElement>::const_iterator end() const { return m_Elements.end(); }
	private:
		std::vector<ResourceElement> m_Elements;
	};

	enum class RendererStageType
	{
		Compute, ForwardGraphics, DeferredGraphics
	};

	struct RendererStage
	{
		std::string Name;
		Ref<Shader> Shader;
		RendererStageType StageType;
		VertexInputLayout VertexInputLayout;
		ResourceLayout Resources;
		Ref<Buffer> VertexBuffer;
		Ref<Buffer> IndexBuffer;

		RendererStage(const std::string& name, Ref<Hog::Shader> shader, RendererStageType type, std::initializer_list<ResourceElement> resources)
			: Name(name), Shader(shader), StageType(type), VertexInputLayout({}), Resources(resources) {}

		RendererStage(const std::string& name, Ref<Hog::Shader> shader, RendererStageType type, std::initializer_list<VertexElement> vertexInput,
			std::initializer_list<ResourceElement> resources, Ref<Buffer> vertexBuffer, Ref<Buffer> indexBuffer)
			: Name(name), Shader(shader), StageType(type), VertexInputLayout(vertexInput), Resources(resources), VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer) {}

		RendererStage() = default;
	};

	struct Node
	{
		static Ref<Node> Create(std::vector<Ref<Node>> parents, RendererStage stageInfo)
		{
			auto ref = CreateRef<Node>(stageInfo);

			for (auto parent : parents)
			{
				parent->AddChild(ref);
				ref->AddParent(parent);
			}

			return ref;
		}

		static Ref<Node> Create(Ref<Node> parent, RendererStage stageInfo)
		{
			auto ref = CreateRef<Node>(stageInfo);
			parent->AddChild(ref);
			ref->AddParent(parent);

			return ref;
		}

		static Ref<Node> Create(RendererStage stageInfo)
		{
			return CreateRef<Node>(stageInfo);
		}

		std::vector<Ref<Node>> ChildList;
		std::vector<Ref<Node>> ParentList;
		RendererStage StageInfo;

		Node(RendererStage stageInfo)
			:StageInfo(stageInfo) {}

		void AddChild(Ref<Node> child)
		{
			ChildList.push_back(child);
		}

		void AddParent(Ref<Node> parent)
		{
			ParentList.push_back(parent);
		}

		bool IsEndNode()
		{
			return ChildList.empty();
		}
	};

	class RenderGraph
	{
	public:
		RenderGraph() = default;
		void Cleanup();

		Ref<Node> AddStage(Ref<Node> parent, RendererStage stageInfo);
		Ref<Node> AddStage(std::vector<Ref<Node>> parents, RendererStage stageInfo);
		std::vector<Ref<Node>> GetStages();
		std::vector<Ref<Node>> GetFinalStages();
	private:
		std::vector<Ref<Node>> m_StartingPoints;
	};
}