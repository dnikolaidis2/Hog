#include "hgpch.h"
#include "Texture.h"

#include "Hog/Utils/RendererUtils.h"
#include "Hog/Renderer/GraphicsContext.h"

namespace Hog {
	Ref<Texture> Texture::Create(Ref<Image> image, SamplerType samplerType)
	{
		return CreateRef<Texture>(image, samplerType);
	}

	Ref<Texture> Texture::Create(Ref<Image> image)
	{
		return Create(image, {});
	}

	Texture::Texture(Ref<Image> image, SamplerType samplerType)
		: m_SamplerType(samplerType), m_Image(image)
	{
		//create a sampler for the texture
		VkSamplerCreateInfo samplerInfo = {};

		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = m_SamplerType.MinFilter;
		samplerInfo.minFilter = m_SamplerType.MagFilter;
		samplerInfo.addressModeU = m_SamplerType.AddressModeU;
		samplerInfo.addressModeV = m_SamplerType.AddressModeV;
		samplerInfo.addressModeW = m_SamplerType.AddressModeW;
		
		if (m_Image->GetLevelCount() > 1)
		{
			samplerInfo.mipmapMode = m_SamplerType.MipMode;
			samplerInfo.minLod = 0.0f; // Optional
			samplerInfo.maxLod = static_cast<float>(m_Image->GetLevelCount());
			samplerInfo.mipLodBias = 0.0f; // Optional
		}
		
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = GraphicsContext::GetGPUInfo()->DeviceProperties2.properties.limits.maxSamplerAnisotropy;

		CheckVkResult(vkCreateSampler(GraphicsContext::GetDevice(), &samplerInfo, nullptr, &m_Sampler));
	}

	Texture::~Texture()
	{
		if (m_Sampler)
			vkDestroySampler(GraphicsContext::GetDevice(), m_Sampler, nullptr);
	}
}