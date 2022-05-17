#pragma once

#include "Hog/Renderer/Shader.h"
#include "Hog/Renderer/Buffer.h"

namespace Hog
{
	enum class ResourceType
	{
		Uniform, Constant, PushConstant, Storage
	};

	struct ResourceElement
	{
		std::string Name;
		ResourceType Type;
		Ref<Buffer> Buffer = nullptr;
		uint32_t ConstantID = 0;
		size_t ConstantSize = 0;
		void* ConstantDataPointer = nullptr;
		uint32_t Binding = 0;
		uint32_t Set = 0;

		ResourceElement(const std::string& name, ResourceType type, Ref<Hog::Buffer> buffer, uint32_t binding, uint32_t set)
			: Name(name), Type(type), Buffer(buffer), Binding(binding), Set(set) {}

		ResourceElement(const std::string& name, ResourceType type, uint32_t constantID, size_t constantSize, void* dataPointer)
			: Name(name), Type(type), ConstantID(constantID), ConstantSize(constantSize), ConstantDataPointer(dataPointer) {}
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
		Compute, Graphics
	};

	struct RendererStage
	{
		std::string Name;
		Ref<Shader> Shader;
		RendererStageType StageType;
		ResourceLayout Resources;
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