#include "vkcpch.h"

#include "Mesh.h"

namespace VulkanCore
{
	Mesh::Mesh(std::string name)
		:m_Name(name), m_Size(0)
	{
		m_Vertices = CreateRef<std::vector<Vertex>>();
	}


	void Mesh::Create()
	{
		m_Size = m_Vertices->size() * sizeof(m_Vertices.get()[0]);
		m_VertexBuffer.Create(m_Size);
		m_VertexBuffer.SetData(m_Vertices->data(), m_Size);
	}

	void Mesh::Destroy()
	{
		m_VertexBuffer.Destroy();
	}
}
