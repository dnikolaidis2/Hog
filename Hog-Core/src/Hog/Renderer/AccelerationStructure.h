#pragma once

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Buffer.h"

namespace Hog{

	class AccelerationStructure
	{
	public:
		static Ref<AccelerationStructure> Create(const std::vector<Ref<Mesh>>& meshes);
		AccelerationStructure(const Ref<Mesh>& primitive);
		AccelerationStructure(const std::vector<Ref<AccelerationStructure>>& bottomLevelStructures);
		~AccelerationStructure();

		VkDeviceAddress GetDeviceAddress() const { return m_DeviceAddress; }
		VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
	private:
		bool m_TopLevel = false;
		Ref<Buffer> m_TransformBuffer;
		Ref<Buffer> m_AccelerationStructureBuffer;
		Ref<Buffer> m_InstanceBuffer;
		VkAccelerationStructureKHR m_Handle;
		VkDeviceAddress m_DeviceAddress;
	};
}

