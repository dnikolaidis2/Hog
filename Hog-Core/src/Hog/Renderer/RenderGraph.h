#pragma once

#include "Hog/Renderer/Shader.h"
#include "Hog/Renderer/Buffer.h"
#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Texture.h"
#include "Hog/Renderer/Types.h"

namespace Hog
{
	struct AttachmentElement
	{
		std::string Name;
		AttachmentType Type;
		Ref<Image> Image;
		bool Clear = false;
		BarrierDescription Barrier;

		AttachmentElement() = default;
		AttachmentElement(std::string name, AttachmentType type, Ref<Hog::Image> image, bool clear = false, BarrierDescription barrier = {})
			: Name(name), Type(type), Image(image), Clear(clear), Barrier(barrier) {}
		AttachmentElement(std::string name, AttachmentType type, bool clear = false, BarrierDescription barrier = {})
			: Name(name), Type(type), Clear(clear), Barrier(barrier) {}
	};

	class AttachmentLayout
	{
	public:
		AttachmentLayout() = default;

		AttachmentLayout(std::initializer_list<AttachmentElement> elements)
			: m_Elements(elements) {}

		const std::vector<AttachmentElement>& GetElements() const { return m_Elements; }
		size_t size() const { return m_Elements.size(); }
		bool ContainsType(AttachmentType type) const;

		std::vector<AttachmentElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<AttachmentElement>::iterator end() { return m_Elements.end(); }
		std::vector<AttachmentElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<AttachmentElement>::const_iterator end() const { return m_Elements.end(); }
		AttachmentElement& operator [](int i) { return m_Elements[i]; }
	private:
		std::vector<AttachmentElement> m_Elements;
	};

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
			: Name(name), Type(type), Size(type.TypeSize()), Offset(0), Normalized(normalized)
		{
		}

		// TODO reaplce with function in DatType struct
		/*uint32_t GetComponentCount() const
		{
			switch (Type)
			{
				case DataType::Defaults::Float:   return 1;
				case DataType::Defaults::Float2:  return 2;
				case DataType::Defaults::Float3:  return 3;
				case DataType::Defaults::Float4:  return 4;
				case DataType::Defaults::Mat3:    return 3; // 3* float3
				case DataType::Defaults::Mat4:    return 4; // 4* float4
				case DataType::Defaults::Int:     return 1;
				case DataType::Defaults::Int2:    return 2;
				case DataType::Defaults::Int3:    return 3;
				case DataType::Defaults::Int4:    return 4;
				case DataType::Defaults::Bool:    return 1;
			}

			HG_CORE_ASSERT(false, "Unknown DataType!");
			return 0;
		}*/
	};

	class VertexInputLayout
	{
	public:
		VertexInputLayout() {}

		VertexInputLayout(std::initializer_list<VertexElement> elements)
			: m_Elements(elements)
		{
			/*CalculateOffsetsAndStride();*/
		}

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<VertexElement>& GetElements() const { return m_Elements; }

		std::vector<VertexElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<VertexElement>::iterator end() { return m_Elements.end(); }
		std::vector<VertexElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<VertexElement>::const_iterator end() const { return m_Elements.end(); }
	private:
		/*void CalculateOffsetsAndStride()
		{
			size_t offset = 0;
			m_Stride = 0;
			for (size_t i = 0; i < m_Elements.size(); i++)
			{
				if (m_Elements[i].Type == DataType::Defaults::Mat3)
				{
					auto name = m_Elements[i].Name;
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float3, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float3, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float3, name, m_Elements[i].Normalized });
					m_Elements.erase(m_Elements.begin() + i);
				}

				if (m_Elements[i].Type == DataType::Defaults::Mat4)
				{
					auto name = m_Elements[i].Name;
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float4, name, m_Elements[i].Normalized });
					m_Elements.insert(m_Elements.begin() + i + 1, { DataType::Defaults::Float4, name, m_Elements[i].Normalized });

					m_Elements.erase(m_Elements.begin() + i);
				}

				auto& element = m_Elements[i];

				element.Location = (uint32_t)i;
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}*/
	private:
		std::vector<VertexElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	struct ResourceElement
	{
		std::string Name;
		ResourceType Type;
		ShaderType BindLocation;
		Ref<Buffer> Buffer = nullptr;
		Ref<Texture> Texture = nullptr;
		std::vector<Ref<Hog::Texture>> Textures;
		uint32_t ConstantID = 0;
		size_t ConstantSize = 0;
		void* ConstantDataPointer = nullptr;
		uint32_t Binding = 0;
		uint32_t Set = 0;
		uint32_t ArrayMaxCount = 0;
		BarrierDescription Barrier;

		ResourceElement(const std::string& name, ResourceType type, ShaderType bindLocation, Ref<Hog::Buffer> buffer, uint32_t set, uint32_t binding, BarrierDescription barrier = {})
			: Name(name), Type(type), BindLocation(bindLocation), Buffer(buffer), Binding(binding), Set(set), Barrier(barrier) {}

		ResourceElement(const std::string& name, ResourceType type, ShaderType bindLocation, Ref<Hog::Texture> texture, uint32_t set, uint32_t binding, BarrierDescription barrier = {})
			: Name(name), Type(type), BindLocation(bindLocation), Texture(texture), Binding(binding), Set(set), Barrier(barrier) {}

		ResourceElement(const std::string& name, ResourceType type, ShaderType bindLocation, const std::vector<Ref<Hog::Texture>>& textures, uint32_t set, uint32_t binding, uint32_t arrayMaxCount,  BarrierDescription barrier = {})
			: Name(name), Type(type), BindLocation(bindLocation), Textures(textures), Binding(binding), Set(set), ArrayMaxCount(arrayMaxCount), Barrier(barrier) {}

		ResourceElement(const std::string& name, ResourceType type, ShaderType bindLocation, uint32_t constantID, size_t constantSize, void* dataPointer)
			: Name(name), Type(type), BindLocation(bindLocation), ConstantID(constantID), ConstantSize(constantSize), ConstantDataPointer(dataPointer) {}

		ResourceElement(const std::string& name, ResourceType type, ShaderType bindLocation, size_t constantSize, void* dataPointer)
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
		size_t size() const { return m_Elements.size(); }
		bool ContainsType(ResourceType type) const;

		std::vector<ResourceElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<ResourceElement>::iterator end() { return m_Elements.end(); }
		std::vector<ResourceElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<ResourceElement>::const_iterator end() const { return m_Elements.end(); }
		const ResourceElement& operator [](int i) const { return m_Elements[i]; }
	private:
		std::vector<ResourceElement> m_Elements;
	};

	struct StageDescription
	{
		std::string Name;
		Ref<Shader> Shader;
		RendererStageType StageType;
		VertexInputLayout VertexInputLayout;
		ResourceLayout Resources;
		std::vector<Ref<Mesh>> Meshes;
		AttachmentLayout Attachments;
		glm::ivec3 GroupCounts = {0, 0, 0};
		Ref<Buffer> DispatchBuffer;
		BarrierDescription BarrierDescription;

		StageDescription(const std::string& name, Ref<Hog::Shader> shader, RendererStageType type, std::initializer_list<ResourceElement> resources, glm::ivec3 groupCounts)
			: Name(name), Shader(shader), StageType(type), VertexInputLayout({}), Resources(resources), GroupCounts(groupCounts) {}

		StageDescription(const std::string& name, RendererStageType type, std::initializer_list<AttachmentElement> attachmentElements)
			: Name(name), StageType(type), Attachments(attachmentElements) {}

		StageDescription(const std::string& name, RendererStageType type, Hog::BarrierDescription description)
			: Name(name), StageType(type), BarrierDescription(description) {}

		StageDescription(const std::string& name, Ref<Hog::Shader> shader, RendererStageType type, std::initializer_list<ResourceElement> resources, std::initializer_list<AttachmentElement> attachmentElements)
			: Name(name), Shader(shader), StageType(type), Resources(resources), Attachments(attachmentElements) {}

		StageDescription(const std::string& name, Ref<Hog::Shader> shader, RendererStageType type, std::initializer_list<VertexElement> vertexInput,
			std::initializer_list<ResourceElement> resources, const std::vector<Ref<Mesh>>& meshes, std::initializer_list<AttachmentElement> attachmentElements)
			: Name(name), Shader(shader), StageType(type), VertexInputLayout(vertexInput), Resources(resources), Meshes(meshes), Attachments(attachmentElements) {}

		StageDescription() = default;
	};

	struct Node
	{
		static Ref<Node> Create(const std::vector<Ref<Node>>& parents, StageDescription stageInfo)
		{
			auto ref = CreateRef<Node>(stageInfo);

			for (auto parent : parents)
			{
				parent->AddChild(ref);
				ref->AddParent(parent);
			}

			return ref;
		}

		static Ref<Node> Create(Ref<Node> parent, StageDescription stageInfo)
		{
			auto ref = CreateRef<Node>(stageInfo);
			parent->AddChild(ref);
			ref->AddParent(parent);

			return ref;
		}

		static Ref<Node> Create(StageDescription stageInfo)
		{
			return CreateRef<Node>(stageInfo);
		}

		std::vector<Ref<Node>> ChildList;
		std::vector<Ref<Node>> ParentList;
		StageDescription StageInfo;

		Node(StageDescription stageInfo)
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

		void Cleanup()
		{
			ChildList.clear();
			ParentList.clear();
		}
	};

	class RenderGraph
	{
	public:
		RenderGraph() = default;
		void Cleanup();

		Ref<Node> AddStage(Ref<Node> parent, StageDescription stageInfo);
		Ref<Node> AddStage(const std::vector<Ref<Node>>& parents, StageDescription stageInfo);
		std::vector<Ref<Node>> GetStages();
		std::vector<Ref<Node>> GetFinalStages();

		bool ContainsStageType(RendererStageType type) const;
	private:
		std::vector<Ref<Node>> m_StartingPoints;
	};
}