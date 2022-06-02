#include "hgpch.h"

#include "Mesh.h"

namespace Hog
{
	MeshPrimitive::MeshPrimitive(const std::vector<Vertex>& vertexData, const std::vector<uint16_t>& indexData)
		: m_Vertices(vertexData), m_Indices(indexData)
	{
	}

	void MeshPrimitive::Build(Ref<Buffer> vertexBuffer, uint64_t vertexOffset, Ref<Buffer> indexBuffer,
		uint64_t indexOffset)
	{
		m_VertexRegion = BufferRegion::Create(vertexBuffer, vertexOffset, sizeof(Vertex) * m_Vertices.size());
		m_VertexRegion->WriteData(m_Vertices.data(), m_VertexRegion->GetSize());
		m_IndexRegion = BufferRegion::Create(indexBuffer, indexOffset, sizeof(uint16_t) * m_Indices.size());
		m_IndexRegion->WriteData(m_Indices.data(), m_IndexRegion->GetSize());
	}

	Ref<Mesh> Mesh::Create(const std::string& name)
	{
		return CreateRef<Mesh>(name);
	}

	void Mesh::AddPrimitive(const std::vector<Vertex>& vertexData, const std::vector<uint16_t>& indexData)
	{
		m_Primitives.emplace_back(vertexData, indexData);
		m_IndexOffsets.push_back(m_IndexBufferSize);
		m_VertexOffsets.push_back(m_VertexBufferSize);

		m_IndexBufferSize += indexData.size() * sizeof(uint16_t);
		m_VertexBufferSize += vertexData.size() * sizeof(Vertex);
	}

	void Mesh::Build()
	{
		m_IndexBuffer = Buffer::Create(BufferDescription::Defaults::IndexBuffer, m_IndexBufferSize);
		m_VertexBuffer = Buffer::Create(BufferDescription::Defaults::VertexBuffer, m_VertexBufferSize);

		for (int i = 0; i < m_Primitives.size(); ++i)
		{
			m_Primitives[i].Build(m_VertexBuffer, m_VertexOffsets[i], m_IndexBuffer, m_IndexOffsets[i]);
		}
	}

	void Mesh::Draw(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION()

		for (auto && primitive: m_Primitives)
		{
			VkBuffer vertexBuffers[] = { m_VertexBuffer->GetHandle() };
			VkDeviceSize offsets[] = { primitive.GetVertexOffset() };

			vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetHandle(), primitive.GetIndexOffset(), VK_INDEX_TYPE_UINT16);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdDrawIndexed(commandBuffer,  static_cast<uint32_t>(primitive.GetIndexCount()), 1, 0, 0, 0);
		}
	}
}
