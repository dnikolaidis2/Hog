#include "hgpch.h"
#include "Light.h"


namespace Hog {
	
	Ref<Light> Light::Create(const LightData& data)
	{
		return CreateRef<Light>(data);
	}

	void Light::UpdateData(Ref<Buffer> buffer, size_t offset)
	{
		m_Region = BufferRegion::Create(buffer, offset, sizeof(LightData));
		UpdateData();
	}

	void Light::UpdateData()
	{
		m_Region->WriteData(&m_Data, sizeof(LightData));
	}
}