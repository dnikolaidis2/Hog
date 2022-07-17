#pragma once

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Buffer.h"

namespace Hog{

	class AccelerationStructure
	{
	public:
		static Ref<AccelerationStructure> Create(const std::vector<Ref<Mesh>>& meshes);
		AccelerationStructure(const MeshPrimitive& primitive, glm::mat4 TransformMatrix = glm::mat4(1.0f));
		AccelerationStructure(const std::vector<Ref<AccelerationStructure>>& bottomLevelStructures);

		VkDeviceAddress GetDeviceAddress() const { return m_DeviceAddress; }
		VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
	private:
		bool m_TopLevel = false;
		Ref<Buffer> m_VertexBuffer;
		Ref<Buffer> m_IndexBuffer;
		Ref<Buffer> m_TransformBuffer;
		Ref<Buffer> m_AccelerationStructureBuffer;
		Ref<Buffer> m_InstanceBuffer;
		VkAccelerationStructureKHR m_Handle;
		VkDeviceAddress m_DeviceAddress;
	};
}

