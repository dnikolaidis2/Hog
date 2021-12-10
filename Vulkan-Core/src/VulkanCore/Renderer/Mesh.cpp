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
		m_VertexBuffer.Create(m_Vertices.size() * sizeof(Vertex));
		m_VertexBuffer.SetData(m_Vertices.data(), m_VertexBuffer.GetSize());
	}

	void Mesh::Destroy()
	{
		m_VertexBuffer.Destroy();
	}

	void Mesh::Draw(VkCommandBuffer commandBuffer)
	{
		std::vector<VkBuffer> vertexBuffers(1);
		std::vector<VkDeviceSize> offsets(1);
		for (int i = 0; i < vertexBuffers.size(); ++i)
		{
			vertexBuffers[i] = m_VertexBuffer.GetHandle();
			offsets[i] = 0;
		}

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());

		vkCmdDraw(commandBuffer, (uint32_t)GetSize(), 1, 0, 0);
	}
}
