#pragma once

#include "Hog/Renderer/RendererObject.h"
#include "Hog/Core/Base.h"
#include "Hog/Renderer/EditorCamera.h"
#include "Hog/Renderer/RenderGraph.h"

namespace Hog
{
	class Renderer
	{
	public:
		struct GlobalShaderData
		{
			glm::vec3 LightPosition;
			int32_t LightMaterialIndex;
		};

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
