#pragma once

namespace Hog
{
	struct RendererStage
	{
		std::string Name;
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

		Ref<Node> AddStage(Ref<Node> parent, RendererStage stageInfo);
		Ref<Node> AddStage(std::vector<Ref<Node>> parents, RendererStage stageInfo);
		std::vector<Ref<Node>> GetStages();
		std::vector<Ref<Node>> GetFinalStages();
	private:
		std::vector<Ref<Node>> m_StartingPoints;
	};
}