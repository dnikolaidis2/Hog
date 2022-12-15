#pragma once

#include <volk.h>
#include "Hog/Math/Math.h"

#include <Hog/Renderer/Types.h>

namespace Hog
{
	class Sampler
	{
	public:
        static Ref<Sampler> Create(const SamplerType& type);
	public:
        Sampler(const SamplerType& type);

        VkSampler GetHandle() const { return m_Handle; }

        operator VkSampler() const { return m_Handle; }

        bool operator==(const Sampler& sampler) const;
	private:
        SamplerType m_SamplerType;
		VkSamplerCreateInfo m_SamplerCreateInfo = {};
        VkSampler m_Handle = VK_NULL_HANDLE;
	};
}