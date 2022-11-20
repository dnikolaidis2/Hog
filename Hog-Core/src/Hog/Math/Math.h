#pragma once

#include <glm/glm.hpp>

namespace Hog::Math
{

	struct Vector3
	{
        // Constants
        static const glm::vec3 Zero;
        static const glm::vec3 One;
        static const glm::vec3 UnitX;
        static const glm::vec3 UnitY;
        static const glm::vec3 UnitZ;
        static const glm::vec3 Up;
        static const glm::vec3 Down;
        static const glm::vec3 Right;
        static const glm::vec3 Left;
        static const glm::vec3 Forward;
        static const glm::vec3 Backward;
	};

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	void CalculateFrustrumCorners(std::vector<glm::vec3>& corners, glm::mat4 projection);

    bool EpsilonCompare(float a, float b);
}
