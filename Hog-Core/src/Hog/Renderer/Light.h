#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "Hog/Renderer/Buffer.h"

namespace Hog {
	enum class LightType : int32_t
	{
		Directional = 0,
		Point = 1,
		Spot = 2,
	};

	/*
	vec3 Position;
	int32 Type;
	vec4 Color;
	vec3 Direction;
	float Intensity;
	*/

	struct alignas(16) LightData
	{
		glm::vec3 Position;
		LightType Type;

		glm::vec4 Color;
		
		glm::vec3 Direction;
		float Intensity;
	};

	class Light
	{
	public:
		static Ref<Light> Create(const LightData& data);
	public:
		Light(const LightData& data)
			: m_Data(data) {}
		void UpdateData(Ref<Buffer>, size_t offset);
		void UpdateData();

		const LightData& GetLightData() { return m_Data; }

		void SetShadowCamera(const Camera& camera) { m_ShadowCamera = camera; }
	private:
		Ref<BufferRegion> m_Region;
		LightData m_Data;
		Camera m_ShadowCamera;
	};
}
