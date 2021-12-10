#include "vkcpch.h"

#include "RendererObject.h"

namespace VulkanCore
{
	Ref<RendererObject> RendererObject::Create(Scope<Mesh> mesh, Ref<Material> material)
	{
		auto ref = CreateRef<RendererObject>();
		ref->SetMesh(std::move(mesh));
		ref->SetMaterial(std::move(material));
		return ref;
	}

	void RendererObject::Render(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr)
	{
		m_Material->Bind(commandBuffer, descriptorSetPtr);

		m_Mesh->Draw(commandBuffer);
	}
}
