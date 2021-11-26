#pragma once

#include <VulkanCore/Renderer/Buffer.h>

namespace VulkanCore
{
	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(std::string name);
		~Mesh() = default;

		void Create();
		void Destroy();
		
		Ref<std::vector<Vertex>> GetVertices() { return m_Vertices; }
		void SetName(std::string name) { m_Name = name; }
		std::string GetName() const { return m_Name; }

		VkBuffer GetBufferHandle() const { return m_VertexBuffer.GetHandle(); }

		uint64_t GetSize() const { return (uint64_t)m_Vertices->size(); }
	private:
		std::string m_Name = "unamed";
		VertexBuffer m_VertexBuffer;
		Ref<std::vector<Vertex>> m_Vertices = CreateRef<std::vector<Vertex>>();
	};
}
