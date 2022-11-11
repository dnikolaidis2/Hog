#pragma once

#include <glm/glm.hpp>

namespace Hog {

	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection, const glm::mat4& view)
			: m_Projection(projection), m_View(view) {}

		virtual ~Camera() = default;

		void SetProjectionMatrix(const glm::mat4& projection) { m_Projection = projection; }
		const glm::mat4& GetProjection() const { return m_Projection; }

		void SetViewMatrix(const glm::mat4& view) { m_View = view; }
		const glm::mat4& GetView() const { return m_View; }

		[[nodiscard]] glm::mat4 GetViewProjection() const { return m_Projection * m_View; }
	protected:
		glm::mat4 m_Projection = glm::mat4(1.0f);
		glm::mat4 m_View = glm::mat4(1.0f);
	};

}