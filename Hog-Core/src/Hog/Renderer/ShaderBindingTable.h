#pragma once

#include "Hog/Renderer/Buffer.h"

namespace Hog
{
	class ShaderBindingTable
	{
	public:
		static Ref<ShaderBindingTable> Create(VkPipeline pipeline);
	public:
		ShaderBindingTable(VkPipeline pipeline);

		const VkStridedDeviceAddressRegionKHR* GetRaygenShaderSBTEntry() {return &m_RaygenShaderSBTEntry;};
		const VkStridedDeviceAddressRegionKHR* GetMissShaderSBTEntry() {return &m_MissShaderSBTEntry;};
		const VkStridedDeviceAddressRegionKHR* GetHitShaderSBTEntry() {return &m_HitShaderSBTEntry;};
		const VkStridedDeviceAddressRegionKHR* GetCallableShaderSBTEntry() {return &m_CallableShaderSBTEntry;};

	private:
		Ref<Buffer> m_RaygenShaderBindingTable;
		Ref<Buffer> m_MissShaderBindingTable;
		Ref<Buffer> m_HitShaderBindingTable;
		Ref<Buffer> m_CallableShaderBindingTable;
		VkStridedDeviceAddressRegionKHR m_RaygenShaderSBTEntry{};
		VkStridedDeviceAddressRegionKHR m_MissShaderSBTEntry{};
		VkStridedDeviceAddressRegionKHR m_HitShaderSBTEntry{};
		VkStridedDeviceAddressRegionKHR m_CallableShaderSBTEntry{};
	};
}
