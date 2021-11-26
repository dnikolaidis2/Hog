#include "vkcpch.h"

#include "Mesh.h"

namespace VulkanCore
{
	Mesh::Mesh(std::string name)
		:m_Name(name)
	{
		m_Vertices = CreateRef<std::vector<Vertex>>();
	}


	void Mesh::Create()
	{
		m_VertexBuffer.Create(m_Vertices->size() * sizeof(Vertex));
		m_VertexBuffer.SetData(m_Vertices->data(), m_VertexBuffer.GetSize());
	}

	void Mesh::Destroy()
	{
		m_VertexBuffer.Destroy();
	}
}
