#include "hgpch.h"
#include "Renderer.h"

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"

namespace Hog
{
	struct RendererState
	{
		
	};

	static RendererState s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		
	}

	void Renderer::BeginScene(const EditorCamera& camera, GlobalShaderData& globalShaderData)
	{
		HG_PROFILE_FUNCTION();
	}

	void Renderer::EndScene()
	{
		HG_PROFILE_FUNCTION();
	}

	void Renderer::Deinitialize()
	{
	}

	void Renderer::DrawObject(const Ref<RendererObject> object)
	{
		HG_PROFILE_FUNCTION();
	}

	void Renderer::DrawObjects(const std::vector<Ref<RendererObject>>& objects)
	{
		HG_PROFILE_FUNCTION();


	}

	Renderer::RendererStats Renderer::GetStats()
	{
		return RendererStats();
	}
}
