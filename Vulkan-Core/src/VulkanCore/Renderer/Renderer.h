#pragma once

#include "VulkanCore/Renderer/RendererObject.h"
#include "VulkanCore/Core/Base.h"
#include "VulkanCore/Renderer/EditorCamera.h"

namespace VulkanCore
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
