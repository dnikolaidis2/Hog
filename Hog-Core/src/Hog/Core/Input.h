#pragma once

#include <glm/glm.hpp>

#include "Hog/Core/KeyCodes.h"
#include "Hog/Core/MouseCodes.h"

namespace Hog {

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
