#pragma once

#include <glm/glm.hpp>
#include "Hog/Renderer/Buffer.h"

namespace Hog {
	enum class LightType : int32_t
	{
		Directional = 0,
	};

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
	private:
		Ref<BufferRegion> m_Region;
		LightData m_Data;
	};
}
