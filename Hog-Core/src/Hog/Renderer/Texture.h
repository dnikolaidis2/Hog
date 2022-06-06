#pragma once

#include "Hog/Renderer/Image.h"

namespace Hog {

	struct SamplerType
	{
		VkFilter MinFilter = VK_FILTER_NEAREST;
		VkFilter MagFilter = VK_FILTER_NEAREST;
		VkSamplerAddressMode AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerAddressMode AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerAddressMode AddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerMipmapMode MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	};

	class Texture
	{
	public:
		static Ref<Texture> Create(SamplerType samplerType, Ref<Image> image);
	public:
		Texture(SamplerType samplerType, Ref<Image> image);
		~Texture();

		VkSampler GetSampler() { return m_Sampler; }
		VkImageView GetImageView() { return m_Image->GetImageView(); }
		VkImageLayout GetImageLayout() { return m_Image->GetImageLayout(); }
		VkFormat GetFormat() const { return m_Image->GetFormat(); }
		void SetGPUIndex(int32_t ind) { m_GPUIndex = ind; }
		int32_t GetGPUIndex() const { return m_GPUIndex; }
		Ref<Image> GetImage() { return m_Image; }
		VkSampleCountFlagBits GetSamples() const { return m_Image->GetSamples(); }
		void ExecuteBarrier(VkCommandBuffer commandBuffer, const BarrierDescription& description) { m_Image->ExecuteBarrier(commandBuffer, description); }
		void SetImageLayout(VkImageLayout layout) { m_Image->SetImageLayout(layout); }
	private:
		Ref<Image> m_Image;
		VkSampler m_Sampler;
		SamplerType m_SamplerType;
		int32_t m_GPUIndex = 0;
	};
}
