#pragma once

#include <glm/glm.hpp>

namespace Hog::Math {

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	void CalculateFrustrumCorners(std::vector<glm::vec3>& corners, glm::mat4 projection);
}
