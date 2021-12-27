#pragma once

#include "VulkanCore/Renderer/RendererObject.h"
#include "VulkanCore/Core/Base.h"
#include "VulkanCore/Renderer/EditorCamera.h"

namespace VulkanCore
{
	class Renderer
	{
	public:
		static void Initialize();
		static void Deinitialize();

		static void BeginScene(const EditorCamera& camera);
		static void EndScene();

		static void DrawObject(const Ref<RendererObject> object);
		static void DrawObjects(const std::vector<Ref<RendererObject>>& objects);
	};
}
