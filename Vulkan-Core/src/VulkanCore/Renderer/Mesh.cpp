#include "vkcpch.h"

#include "Mesh.h"

namespace VulkanCore
{
	Scope<Mesh> Mesh::Create()
	{
		return CreateScope<Mesh>();
	}

	Mesh::Mesh(std::string name)
		:m_Name(name)
	{
	}


	void Mesh::Load()
	{
		m_VertexBuffer = VertexBuffer::Create((uint32_t)(m_Vertices.size() * sizeof(Vertex)));
		m_VertexBuffer->SetData(m_Vertices.data(), m_VertexBuffer->GetSize());
	}

	void Mesh::Destroy()
	{
		m_VertexBuffer.reset();
	}

	void Mesh::Draw(VkCommandBuffer commandBuffer)
	{
		VKC_PROFILE_FUNCTION()

		VkBuffer vertexBuffers[] = { m_VertexBuffer->GetHandle() };
		VkDeviceSize offsets[] = { 0 };
		
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdDraw(commandBuffer, (uint32_t)GetSize(), 1, 0, 0);
	}
}
