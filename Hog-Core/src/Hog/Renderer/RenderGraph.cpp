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
		for (const auto & elem : m_Elements)
		{
			if (elem.Type == type) return true;
		}

		return false;
	}

	void RenderGraph::Cleanup()
	{
		m_StartingPoints.clear();
	}

	Ref<Node> RenderGraph::AddStage(Ref<Node> parent, RendererStage stageInfo)
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

	Ref<Node> RenderGraph::AddStage(std::vector<Ref<Node>> parents, RendererStage stageInfo)
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

		while(!toVisit.empty())
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
}
