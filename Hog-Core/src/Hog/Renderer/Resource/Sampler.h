#pragma once

#include <volk.h>
#include "Hog/Math/Math.h"

#include <boost/functional/hash.hpp>

namespace Hog
{
    struct SamplerType
    {
        VkFilter MinFilter = VK_FILTER_NEAREST;
        VkFilter MagFilter = VK_FILTER_NEAREST;
        VkSamplerAddressMode AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode AddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerMipmapMode MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    };

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

template<>
struct std::hash<VkSamplerCreateInfo>
{
	std::size_t operator()(const VkSamplerCreateInfo& s) const noexcept
	{
        std::size_t seed = 0;

        boost::hash_combine(seed, s.sType);
        boost::hash_combine(seed, s.flags);
        boost::hash_combine(seed, s.magFilter);
        boost::hash_combine(seed, s.minFilter);
        boost::hash_combine(seed, s.mipmapMode);
        boost::hash_combine(seed, s.addressModeU);
        boost::hash_combine(seed, s.addressModeV);
        boost::hash_combine(seed, s.addressModeW);
        boost::hash_combine(seed, s.mipLodBias);
        boost::hash_combine(seed, s.anisotropyEnable);
        boost::hash_combine(seed, s.maxAnisotropy);
        boost::hash_combine(seed, s.compareEnable);
        boost::hash_combine(seed, s.compareOp);
        boost::hash_combine(seed, s.minLod);
        boost::hash_combine(seed, s.maxLod);
        boost::hash_combine(seed, s.borderColor);
        boost::hash_combine(seed, s.unnormalizedCoordinates);

        return seed;
	}
};