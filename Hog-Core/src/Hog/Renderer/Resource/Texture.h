#pragma once

#include "Hog/Renderer/Resource/Image.h"
#include "Hog/Renderer/Resource/Sampler.h"

namespace Hog {

	class Texture
	{
	public:
		static Ref<Texture> Create(Ref<Image> image, SamplerType samplerType);
		static Ref<Texture> Create(SamplerType samplerType);
		static Ref<Texture> Create(Ref<Image> image);
	public:
		Texture(Ref<Image> image, SamplerType samplerType);
		Texture(SamplerType samplerType);
		~Texture();

		VkSampler GetSampler() { return m_Sampler->GetHandle(); }
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
		Ref<Sampler> m_Sampler;
		Ref<Image> m_Image;
		int32_t m_GPUIndex = 0;
	};
}
