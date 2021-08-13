#pragma once

#include <glm/glm.hpp>

#include "VulkanCore/Core/KeyCodes.h"
#include "VulkanCore/Core/MouseCodes.h"

namespace VulkanCore {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
