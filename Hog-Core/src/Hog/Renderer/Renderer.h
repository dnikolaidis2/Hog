#pragma once

#include "Hog/Renderer/RenderGraph.h"

namespace Hog
{
	class Renderer
	{
	public:
		static void Initialize(RenderGraph renderGraph);
		static void Deinitialize();

		static void Draw();

		struct RendererStats
		{
			uint64_t FrameCount = 0;
		};

		static RendererStats GetStats();
	};
}
