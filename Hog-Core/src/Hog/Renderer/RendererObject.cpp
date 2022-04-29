#include "hgpch.h"

#include "RendererObject.h"

namespace Hog
{
	Ref<RendererObject> RendererObject::Create(Scope<Mesh> mesh, Ref<Material> material)
	{
		auto ref = CreateRef<RendererObject>();
		ref->SetMesh(std::move(mesh));
		ref->SetMaterial(std::move(material));
		return ref;
	}

	void RendererObject::Draw(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION()

		m_Mesh->Draw(commandBuffer);
	}
}
