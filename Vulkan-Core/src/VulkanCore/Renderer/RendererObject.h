#pragma once

#include <VulkanCore/Core/Base.h>
#include <VulkanCore/Renderer/Mesh.h>
#include <VulkanCore/Renderer/Material.h>

namespace VulkanCore
{
	class RendererObject
	{
	public:
		static Ref<RendererObject> Create(Scope<Mesh> mesh, Ref<Material> material);

		RendererObject() = default;

		void SetMesh(Scope<Mesh> mesh) { m_Mesh = std::move(mesh); }
		void SetMaterial(Ref<Material> material) { m_Material = material; }
		void SetTransform(glm::mat4 transform) { m_Transform = transform; }
		void Transform(glm::mat4 transform) { m_Transform *= transform; }

		std::string GetName() const { return m_Mesh->GetName(); }
		Ref<Material> GetMaterial() const { return m_Material; }
		glm::mat4& GetTransform() { return m_Transform; }

		void Draw(VkCommandBuffer commandBuffer);
	private:
		Scope<Mesh> m_Mesh;
		Ref<Material> m_Material;

		glm::mat4 m_Transform = glm::mat4(1.0f);
	};
}