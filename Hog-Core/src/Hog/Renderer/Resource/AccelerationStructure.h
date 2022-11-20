#pragma once

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Resource/Buffer.h"

namespace Hog{

	class AccelerationStructure
	{
	public:
		static Ref<AccelerationStructure> Create(const std::vector<Ref<Mesh>>& meshes);
		AccelerationStructure(const std::vector<Ref<Mesh>>& meshes);
		AccelerationStructure(const Ref<AccelerationStructure>& bottomLevelStructure);
		~AccelerationStructure();

		VkDeviceAddress GetDeviceAddress() const { return m_DeviceAddress; }
		VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
		const VkAccelerationStructureKHR* GetHandlePtr() const { return &m_Handle; }
	private:
		bool m_TopLevel = false;
		Ref<Buffer> m_AccelerationStructureBuffer;
		
		Ref<Buffer> m_InstanceBuffer;

		std::vector<Ref<Mesh>> m_Meshes;
		std::vector<Ref<Buffer>> m_TransformBuffers;
		
		Ref<AccelerationStructure> m_BottomLevelAS;

		VkAccelerationStructureKHR m_Handle;
		VkDeviceAddress m_DeviceAddress;
	};
}

