#pragma once

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Buffer.h"

namespace Hog{

	class AccelerationStructure
	{
	public:
		static Ref<AccelerationStructure> Create(const std::vector<Ref<Mesh>>& meshes);
		AccelerationStructure(MeshPrimitive);
	private:
		bool m_TopLevel = false;
		Ref<Buffer> m_VertexBuffer;
		Ref<Buffer> m_IndexBuffer;
		Ref<Buffer> m_AccelerationStructureBuffer;
		VkAccelerationStructureKHR m_Handle;
		VkDeviceAddress m_DeviceAddress;
	};
}

