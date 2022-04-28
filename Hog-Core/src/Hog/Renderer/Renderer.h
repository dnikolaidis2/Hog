#pragma once

#include "Hog/Renderer/RendererObject.h"
#include "Hog/Core/Base.h"
#include "Hog/Renderer/EditorCamera.h"

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

		static void Initialize();
		static void Deinitialize();

		static void BeginScene(const EditorCamera& camera, GlobalShaderData& globalShaderData);
		static void EndScene();

		static void DrawObject(const Ref<RendererObject> object);
		static void DrawObjects(const std::vector<Ref<RendererObject>>& objects);

		struct RendererStats
		{
			uint64_t FrameCount = 0;
		};

		static RendererStats GetStats();
	};
}
