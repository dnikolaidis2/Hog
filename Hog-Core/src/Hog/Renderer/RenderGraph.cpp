#include "hgpch.h"

#include "RenderGraph.h"

namespace Hog
{
	bool AttachmentLayout::ContainsType(AttachmentType type) const
	{
		for (const auto& elem : m_Elements)
		{
			if (elem.Type == type) return true;
		}

		return false;
	}

	bool ResourceLayout::ContainsType(ResourceType type) const
	{
		for (const auto& elem : m_Elements)
		{
			if (elem.Type == type) return true;
		}

		return false;
	}

	void RenderGraph::Cleanup()
	{
		std::stack<Ref<Node>> toVisit;
		for (auto node : m_StartingPoints)
		{
			toVisit.push(node);
		}

		m_StartingPoints.clear();

		while (!toVisit.empty())
		{
			Ref<Node> visiting = toVisit.top();
			toVisit.pop();

			for (auto child : visiting->ChildList)
			{
				child->ParentList.erase(std::find(child->ParentList.begin(), child->ParentList.end(), visiting));
				toVisit.push(child);
			}

			visiting->Cleanup();
		}
	}

	Ref<Node> RenderGraph::AddStage(Ref<Node> parent, StageDescription stageInfo)
	{
		if (parent == nullptr)
		{
			auto ref = Node::Create(stageInfo);
			m_StartingPoints.push_back(ref);
			return ref;
		}
		else
		{
			auto ref = Node::Create(parent, stageInfo);
			return ref;
		}
	}

	Ref<Node> RenderGraph::AddStage(const std::vector<Ref<Node>>& parents, StageDescription stageInfo)
	{
		return Node::Create(parents, stageInfo);
	}

	std::vector<Ref<Node>> RenderGraph::GetStages()
	{
		std::vector<Ref<Node>> stages;
		std::queue<Ref<Node>> toVisit;
		for (auto node : m_StartingPoints)
		{
			toVisit.push(node);
		}

		while (!toVisit.empty())
		{
			Ref<Node> visiting = toVisit.front();

			if (std::find(stages.begin(), stages.end(), visiting) == stages.end())
			{
				stages.push_back(visiting);
			}

			for (auto child : visiting->ChildList)
			{
				toVisit.push(child);
			}

			toVisit.pop();
		}

		return stages;
	}

	std::vector<Ref<Node>> RenderGraph::GetFinalStages()
	{
		std::vector<Ref<Node>> stages;
		std::stack<Ref<Node>> toVisit;
		for (auto node : m_StartingPoints)
		{
			toVisit.push(node);
		}

		while (!toVisit.empty())
		{
			Ref<Node> visiting = toVisit.top();
			toVisit.pop();

			if (visiting->IsEndNode() &&
				std::find(stages.begin(), stages.end(), visiting) == stages.end())
			{
				stages.push_back(visiting);
			}

			for (auto child : visiting->ChildList)
			{
				toVisit.push(child);
			}
		}

		return stages;
	}

	bool RenderGraph::ContainsStageType(RendererStageType type) const
	{
		std::queue<Ref<Node>> toVisit;
		for (auto node : m_StartingPoints)
		{
			toVisit.push(node);
		}

		while (!toVisit.empty())
		{
			Ref<Node> visiting = toVisit.front();

			if (visiting->StageInfo.StageType == type)
			{
				return true;
			}

			for (auto child : visiting->ChildList)
			{
				toVisit.push(child);
			}

			toVisit.pop();
		}

		return false;
	}
}