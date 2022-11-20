#include "hgpch.h"

#include "Sampler.h"

#include "Hog/Utils/RendererUtils.h"
#include "Hog/Renderer/GraphicsContext.h"

namespace Hog
{
	Ref<Sampler> Sampler::Create(const SamplerType& type)
	{
		return CreateRef<Sampler>(type);
	}

	Sampler::Sampler(const SamplerType& type)
		: m_SamplerType(type)
	{
		m_SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		m_SamplerCreateInfo.magFilter = m_SamplerType.MinFilter;
		m_SamplerCreateInfo.minFilter = m_SamplerType.MagFilter;
		m_SamplerCreateInfo.addressModeU = m_SamplerType.AddressModeU;
		m_SamplerCreateInfo.addressModeV = m_SamplerType.AddressModeV;
		m_SamplerCreateInfo.addressModeW = m_SamplerType.AddressModeW;
		
		m_SamplerCreateInfo.mipmapMode = m_SamplerType.MipMode;
		m_SamplerCreateInfo.minLod = 0.0f; // Optional
		//m_SamplerCreateInfo.maxLod = static_cast<float>(m_Image->GetLevelCount());
		m_SamplerCreateInfo.mipLodBias = 0.0f; // Optional

		m_SamplerCreateInfo.anisotropyEnable = VK_TRUE;
		m_SamplerCreateInfo.maxAnisotropy = GraphicsContext::GetGPUInfo()->DeviceProperties2.properties.limits.maxSamplerAnisotropy;

		CheckVkResult(vkCreateSampler(GraphicsContext::GetDevice(), &m_SamplerCreateInfo, nullptr, &m_Handle));
	}

	bool Sampler::operator==(const Sampler& sampler) const
	{
		return std::hash<VkSamplerCreateInfo>{}(m_SamplerCreateInfo) == std::hash<VkSamplerCreateInfo>{}(sampler.m_SamplerCreateInfo);
	}
}
