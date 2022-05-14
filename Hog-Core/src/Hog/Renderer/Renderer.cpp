#include "hgpch.h"
#include "Renderer.h"

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Core/CVars.h"

AutoCVar_Int CVar_ImageMipLevels("renderer.enableMipMapping", "Enable mip mapping for textures", 0, CVarFlags::None);

namespace Hog
{
	struct RendererState
	{
		RenderGraph Graph;
	};

	static RendererState s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.Graph = renderGraph;
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
