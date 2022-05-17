// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <unordered_map>

namespace Hog {


	class DescriptorAllocator {
	public:

		struct PoolSizes {
			std::vector<std::pair<VkDescriptorType, float>> Sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		void ResetPools();
		bool Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

		void Init(VkDevice newDevice);

		void Cleanup();

		VkDevice Device;
	private:
		VkDescriptorPool GrabPool();

		VkDescriptorPool m_CurrentPool{ VK_NULL_HANDLE };
		PoolSizes m_DescriptorSizes;
		std::vector<VkDescriptorPool> m_UsedPools;
		std::vector<VkDescriptorPool> m_FreePools;
	};


	class DescriptorLayoutCache {
	public:
		void Init(VkDevice newDevice);
		void Cleanup();

		VkDescriptorSetLayout CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo {
			//good idea to turn this into a inlined array
			std::vector<VkDescriptorSetLayoutBinding> Bindings;

			bool operator==(const DescriptorLayoutInfo& other) const;

			size_t Hash() const;
		};



	private:

		struct DescriptorLayoutHash
		{

			std::size_t operator()(const DescriptorLayoutInfo& k) const
			{
				return k.Hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_LayoutCache;
		VkDevice m_Device;
	};


	class DescriptorBuilder {
	public:

		static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

		DescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

		bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
		bool Build(VkDescriptorSet& set);
	private:

		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;


		DescriptorLayoutCache* m_Cache;
		DescriptorAllocator* m_Alloc;
	};
}

