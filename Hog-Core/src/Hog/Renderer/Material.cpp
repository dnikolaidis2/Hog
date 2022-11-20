#include "hgpch.h"

#include "Material.h"

#include "Hog/Core/CVars.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Resource/Buffer.h"

namespace Hog
{
	Ref<Material> Material::Create(const std::string& name, MaterialData& data)
	{
		HG_PROFILE_FUNCTION()

		return CreateRef<Material>(name, data);
	}

	void Material::UpdateData(Ref<Buffer> buffer, size_t offset)
	{
		m_Region = BufferRegion::Create(buffer, offset, sizeof(MaterialGPUData));
		UpdateData();
	}

	void Material::UpdateData()
	{
		MaterialGPUData data (m_Data);
		m_Region->WriteData(&data, m_Region->GetSize());
	}
}
