#include "hgpch.h"
#include "ShaderBindingTable.h"

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"

namespace Hog {

	Ref<ShaderBindingTable> ShaderBindingTable::Create(VkPipeline pipeline)
	{
		return CreateRef<ShaderBindingTable>(pipeline);
	}

	ShaderBindingTable::ShaderBindingTable(VkPipeline pipeline)
	{
		const uint32_t handleSize = GraphicsContext::GetGPUInfo()->RayTracingPipelineProperties.shaderGroupHandleSize;
		auto alignedSize = [](uint32_t value, uint32_t alignment) -> uint32_t
		{
			return (value + alignment - 1) & ~(alignment - 1);
		};
		uint32_t handleSizeAligned = alignedSize(GraphicsContext::GetGPUInfo()->RayTracingPipelineProperties.shaderGroupHandleSize,
			GraphicsContext::GetGPUInfo()->RayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(3);
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		CheckVkResult(vkGetRayTracingShaderGroupHandlesKHR(GraphicsContext::GetDevice(), pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

		// Copy handles
		m_RaygenShaderBindingTable = Buffer::Create(BufferDescription::Defaults::ShaderBindingTable, handleSize);
		m_RaygenShaderBindingTable->WriteData(shaderHandleStorage.data(), handleSize);

		m_RaygenShaderSBTEntry.deviceAddress = m_RaygenShaderBindingTable->GetBufferDeviceAddress();
		m_RaygenShaderSBTEntry.stride = handleSizeAligned;
		m_RaygenShaderSBTEntry.size = handleSizeAligned;

		m_MissShaderBindingTable = Buffer::Create(BufferDescription::Defaults::ShaderBindingTable, handleSize);
		m_MissShaderBindingTable->WriteData(shaderHandleStorage.data() + handleSizeAligned, handleSize);
		
		m_MissShaderSBTEntry.deviceAddress = m_MissShaderBindingTable->GetBufferDeviceAddress();
		m_MissShaderSBTEntry.stride = handleSizeAligned;
		m_MissShaderSBTEntry.size = handleSizeAligned;
		
		m_HitShaderBindingTable = Buffer::Create(BufferDescription::Defaults::ShaderBindingTable, handleSize);
		m_HitShaderBindingTable->WriteData(shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);

		m_HitShaderSBTEntry.deviceAddress = m_HitShaderBindingTable->GetBufferDeviceAddress();
		m_HitShaderSBTEntry.stride = handleSizeAligned;
		m_HitShaderSBTEntry.size = handleSizeAligned;
	}
}