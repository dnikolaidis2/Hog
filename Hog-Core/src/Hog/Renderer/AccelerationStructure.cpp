#include "hgpch.h"

#include "AccelerationStructure.h"

#include "Hog/Renderer/GraphicsContext.h"

namespace Hog
{
	Ref<AccelerationStructure> AccelerationStructure::Create(const std::vector<Ref<Mesh>>& meshes)
	{
		std::vector<Ref<AccelerationStructure>> bottomLevelStructures;
		size_t indexOffset = 0;
		for (auto mesh : meshes)
		{
			bottomLevelStructures.push_back(CreateRef<AccelerationStructure>(mesh));
		}

		return CreateRef<AccelerationStructure>(bottomLevelStructures);
	}

	AccelerationStructure::AccelerationStructure(const Ref<Mesh>& mesh)
	{
		glm::mat4 transformMatrix = mesh->GetModelMatrix();
		m_TransformBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureBuildInput, sizeof(VkTransformMatrixKHR));
		m_TransformBuffer->WriteData(&transformMatrix, sizeof(VkTransformMatrixKHR));

		std::vector<VkAccelerationStructureGeometryKHR> accelerationStructureGeometries;
		std::vector<uint32_t> triangleCounts;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfoPointers;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> accelerationBuildStructureRangeInfos;

		for (auto primitive = mesh->begin(); primitive != mesh->end(); primitive++)
		{
			VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
			accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
			accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = mesh->GetVertexBuffer()->GetBufferDeviceAddress() + primitive->GetVertexOffset();
			accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
			accelerationStructureGeometry.geometry.triangles.maxVertex = primitive->GetVertexCount();
			accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
			accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = mesh->GetIndexBuffer()->GetBufferDeviceAddress();
			accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = m_TransformBuffer->GetBufferDeviceAddress();
			accelerationStructureGeometries.push_back(accelerationStructureGeometry);
			triangleCounts.push_back(primitive->GetIndexCount() / 3);

			VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
			accelerationStructureBuildRangeInfo.primitiveCount = primitive->GetIndexCount() / 3;
			accelerationStructureBuildRangeInfo.primitiveOffset = primitive->GetIndexOffset();
			accelerationStructureBuildRangeInfo.firstVertex = 0;
			accelerationStructureBuildRangeInfo.transformOffset = 0;
			accelerationBuildStructureRangeInfos.push_back(accelerationStructureBuildRangeInfo);
		}

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(accelerationStructureGeometries.size());
		accelerationStructureBuildGeometryInfo.pGeometries = accelerationStructureGeometries.data();

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
			GraphicsContext::GetDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			triangleCounts.data(),
			&accelerationStructureBuildSizesInfo);

		m_AccelerationStructureBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructure, accelerationStructureBuildSizesInfo.accelerationStructureSize);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = m_AccelerationStructureBuffer->GetHandle();
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		vkCreateAccelerationStructureKHR(GraphicsContext::GetDevice(), &accelerationStructureCreateInfo, nullptr, &m_Handle);

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		auto scratchBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureScratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize);

		accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationStructureBuildGeometryInfo.dstAccelerationStructure = m_Handle;
		accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->GetBufferDeviceAddress();

		for (int i = 0; i < accelerationBuildStructureRangeInfos.size(); i++)
		{
			accelerationBuildStructureRangeInfoPointers.push_back(&accelerationBuildStructureRangeInfos[i]);
		}

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer) {
			vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationStructureBuildGeometryInfo,
				accelerationBuildStructureRangeInfoPointers.data());
		});

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = m_Handle;
		m_DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GraphicsContext::GetDevice(), &accelerationDeviceAddressInfo);
	}

	AccelerationStructure::AccelerationStructure(const std::vector<Ref<AccelerationStructure>>& bottomLevelStructures)
		:m_TopLevel(true)
	{
		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f 
		};

		std::vector<VkAccelerationStructureInstanceKHR> structureInstances(bottomLevelStructures.size());
		std::transform(bottomLevelStructures.begin(), bottomLevelStructures.end(), structureInstances.begin(), 
			[transformMatrix](Ref<AccelerationStructure> structure ){
				VkAccelerationStructureInstanceKHR instance{};
				instance.transform = transformMatrix;
				instance.instanceCustomIndex = 0;
				instance.mask = 0xFF;
				instance.instanceShaderBindingTableRecordOffset = 0;
				instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
				instance.accelerationStructureReference = structure->GetDeviceAddress();
				return instance;
			});
		
		m_InstanceBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureBuildInput, structureInstances.size() + sizeof(VkAccelerationStructureInstanceKHR));
		m_InstanceBuffer->WriteData(structureInstances.data(), structureInstances.size() + sizeof(VkAccelerationStructureInstanceKHR));

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_TRUE;
		accelerationStructureGeometry.geometry.instances.data.deviceAddress = m_InstanceBuffer->GetBufferDeviceAddress();

		// Get size info
		/*
		The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
		*/
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = static_cast<uint32_t>(structureInstances.size());

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
			GraphicsContext::GetDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&primitive_count,
			&accelerationStructureBuildSizesInfo);

		m_AccelerationStructureBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructure, accelerationStructureBuildSizesInfo.accelerationStructureSize);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = m_AccelerationStructureBuffer->GetHandle();
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		vkCreateAccelerationStructureKHR(GraphicsContext::GetDevice(), &accelerationStructureCreateInfo, nullptr, &m_Handle);

		// Create a small scratch buffer used during build of the top level acceleration structure
		auto scratchBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureScratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = m_Handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->GetBufferDeviceAddress();

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = 1;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		GraphicsContext::ImmediateSubmit(
			[=](VkCommandBuffer commandBuffer) {
				vkCmdBuildAccelerationStructuresKHR(
					commandBuffer,
					1,
					&accelerationBuildGeometryInfo,
					accelerationBuildStructureRangeInfos.data());
			});

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = m_Handle;
		m_DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GraphicsContext::GetDevice(), &accelerationDeviceAddressInfo);
	}
}
