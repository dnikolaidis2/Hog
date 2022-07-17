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
			bottomLevelStructures.resize(bottomLevelStructures.size() + mesh->GetPrimitiveCount());
			std::transform(mesh->begin(), mesh->end(), bottomLevelStructures.begin() + indexOffset,
				[&](const MeshPrimitive& primitive) {
					return CreateRef<AccelerationStructure>(primitive, mesh->GetModelMatrix());
				});
			indexOffset += mesh->GetPrimitiveCount();
		}

		return CreateRef<AccelerationStructure>(bottomLevelStructures);
	}

	AccelerationStructure::AccelerationStructure(const MeshPrimitive& primitive, glm::mat4 TransformMatrix)
	{
		const auto& primitiveVertices = primitive.GetVertices();
		const auto& primitiveIndicies = primitive.GetIndices();

		struct TriangleVertex
		{
			glm::vec3 Position;
		};

		std::vector<TriangleVertex> vertices(primitiveVertices.size());
		for (size_t i = 0; i < primitiveVertices.size(); i++)
		{
			vertices[i].Position = primitiveVertices[i].Position;
		}

		m_VertexBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureBuildInput, static_cast<uint64_t>(vertices.size() * sizeof(TriangleVertex)));
		m_VertexBuffer->WriteData(vertices.data(), static_cast<uint64_t>(vertices.size() * sizeof(TriangleVertex)));

		m_IndexBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureBuildInput, primitiveIndicies.size() * sizeof(uint16_t));
		m_IndexBuffer->WriteData(const_cast<uint16_t*>(primitiveIndicies.data()), primitiveIndicies.size() * sizeof(uint16_t));

		m_TransformBuffer = Buffer::Create(BufferDescription::Defaults::AccelerationStructureBuildInput, sizeof(VkTransformMatrixKHR));
		m_TransformBuffer->WriteData(&TransformMatrix, sizeof(VkTransformMatrixKHR));

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = m_VertexBuffer->GetBufferDeviceAddress();
		accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
		accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
		accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
		accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = m_IndexBuffer->GetBufferDeviceAddress();
		//accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
		//accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
		accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = m_TransformBuffer->GetBufferDeviceAddress();

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		const uint32_t numTriangles = static_cast<uint32_t>(primitiveIndicies.size() / 3);
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
			GraphicsContext::GetDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&numTriangles,
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

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = m_Handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->GetBufferDeviceAddress();

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		GraphicsContext::ImmediateSubmit([=](VkCommandBuffer commandBuffer) {
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
