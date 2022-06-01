#pragma once

#include <Hog/Renderer/Buffer.h>

namespace Hog
{
	class MeshPrimitive
	{
	public:
		MeshPrimitive(const std::vector<Vertex>& vertexData, const std::vector<uint16_t>& indexData);

		void Build(Ref<Buffer> vertexBuffer, uint64_t vertexOffset, Ref<Buffer> indexBuffer, uint64_t indexOffset);

		uint64_t GetVertexDataSize() const { return m_Vertices.size() * sizeof(Vertex); }
		uint64_t GetIndexDataSize() const { return m_Indices.size() * sizeof(uint16_t); }

		uint32_t GetVertexCount() const { return m_Vertices.size(); }
		uint32_t GetIndexCount() const { return m_Indices.size(); }

		uint64_t GetVertexOffset() const { return m_VertexRegion->GetOffset(); }
		uint64_t GetIndexOffset() const { return m_IndexRegion->GetOffset(); }

		Ref<BufferRegion> GetVertexRegion() { return m_VertexRegion; }
		Ref<BufferRegion> GetIndexRegion() { return m_IndexRegion; }

		void SetVertexRegion(Ref<BufferRegion> vertexRegion) { m_VertexRegion = std::move(vertexRegion); }
		void SetIndexRegion(Ref<BufferRegion> indexRegion) { m_VertexRegion = std::move(indexRegion); }

		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<uint16_t>& GetIndices() const { return m_Indices; }
	public:
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;

		Ref<BufferRegion> m_VertexRegion;
		Ref<BufferRegion> m_IndexRegion;
	};

	class Mesh
	{
	public:
		static Ref<Mesh> Create(const std::string& name);

		Mesh(const std::string& name)
			: m_Name(name) {}
		~Mesh() = default;

		void AddPrimitive(const std::vector<Vertex>& vertexData, const std::vector<uint16_t>& indexData);
		void Build();

		void SetModelMatrix(glm::mat4 matrix) { m_ModelMatrix = matrix; }
		glm::mat4 GetModelMatrix() const { return m_ModelMatrix; }

		void Draw(VkCommandBuffer commandBuffer);
	private:
		std::string m_Name;
		std::vector<MeshPrimitive> m_Primitives;

		Ref<Buffer> m_VertexBuffer;
		Ref<Buffer> m_IndexBuffer;

		std::vector<uint64_t> m_VertexOffsets;
		std::vector<uint64_t> m_IndexOffsets;

		uint32_t m_VertexBufferSize = 0;
		uint32_t m_IndexBufferSize = 0;

		glm::mat4 m_ModelMatrix = glm::mat4(1.0f);
	};
}
