#pragma once

#include "Image.h"
#include "Hog/Renderer/RenderGraph.h"

namespace Hog
{
	class Renderer
	{
	public:
		static void Initialize(RenderGraph renderGraph);
		static void Deinitialize();
		static void SetFinalRenderTarget(Ref<Image> image);
		static Ref<Image> GetFinalRenderTarget();

		static void Draw();

		struct RendererStats
		{
			uint64_t FrameCount = 0;
		};

		static RendererStats GetStats();
	};
}
