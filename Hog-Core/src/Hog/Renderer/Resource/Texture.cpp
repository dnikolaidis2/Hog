#include "hgpch.h"
#include "Texture.h"

#include "Hog/Utils/RendererUtils.h"
#include "Hog/Renderer/GraphicsContext.h"

namespace Hog {
	Ref<Texture> Texture::Create(Ref<Image> image, SamplerType samplerType)
	{
		return CreateRef<Texture>(image, samplerType);
	}

	Ref<Texture> Texture::Create(SamplerType samplerType)
	{
		return CreateRef<Texture>(samplerType);
	}

	Ref<Texture> Texture::Create(Ref<Image> image)
	{
		return Create(image, {});
	}

	Texture::Texture(Ref<Image> image, SamplerType samplerType)
		: m_Image(image)
	{
		//create a sampler for the texture
		VkSamplerCreateInfo samplerInfo = {};

		
	}

	Texture::Texture(SamplerType samplerType)
	{
		
	}

	Texture::~Texture()
	{
		//if (m_Sampler)
			//vkDestroySampler(GraphicsContext::GetDevice(), , nullptr);
	}
}