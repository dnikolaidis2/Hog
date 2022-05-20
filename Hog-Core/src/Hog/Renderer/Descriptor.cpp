#include "hgpch.h"

#include "Descriptor.h"

#include <algorithm>

#include "Hog/Utils/RendererUtils.h"

namespace Hog {


	VkDescriptorPool CreatePool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.Sizes.size());
		for (auto sz : poolSizes.Sizes) {
			sizes.push_back({ sz.first, uint32_t(sz.second * count) });
		}
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = flags;
		pool_info.maxSets = count;
		pool_info.poolSizeCount = (uint32_t)sizes.size();
		pool_info.pPoolSizes = sizes.data();

		VkDescriptorPool descriptorPool;
		CheckVkResult(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool));

		return descriptorPool;
	}

	void DescriptorAllocator::ResetPools()
	{
		for (auto p : m_UsedPools)
		{
			CheckVkResult(vkResetDescriptorPool(Device, p, 0));
		}

		m_FreePools = m_UsedPools;
		m_UsedPools.clear();
		m_CurrentPool = VK_NULL_HANDLE;
	}

	bool DescriptorAllocator::Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
	{
		if (m_CurrentPool == VK_NULL_HANDLE)
		{
			m_CurrentPool = GrabPool();
			m_UsedPools.push_back(m_CurrentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorPool = m_CurrentPool;
		allocInfo.descriptorSetCount = 1;


		VkResult allocResult = vkAllocateDescriptorSets(Device, &allocInfo, set);
		bool needReallocate = false;

		switch (allocResult) {
		case VK_SUCCESS:
			//all good, return
			return true;

			break;
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			//reallocate pool
			needReallocate = true;
			break;
		default:
			//unrecoverable error
			return false;
		}

		if (needReallocate)
		{
			//Allocate a new pool and retry
			m_CurrentPool = GrabPool();
			m_UsedPools.push_back(m_CurrentPool);

			allocResult = vkAllocateDescriptorSets(Device, &allocInfo, set);

			//if it still fails then we have big issues
			if (allocResult == VK_SUCCESS)
			{
				return true;
			}
		}

		return false;
	}

	void DescriptorAllocator::Init(VkDevice newDevice)
	{
		Device = newDevice;
	}

	void DescriptorAllocator::Cleanup()
	{
		//delete every pool held
		for (auto p : m_FreePools)
		{
			vkDestroyDescriptorPool(Device, p, nullptr);
		}
		for (auto p : m_UsedPools)
		{
			vkDestroyDescriptorPool(Device, p, nullptr);
		}
	}

	VkDescriptorPool DescriptorAllocator::GrabPool()
	{
		if (m_FreePools.size() > 0)
		{
			VkDescriptorPool pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		else {
			return CreatePool(Device, m_DescriptorSizes, 1000, 0);
		}
	}


	void DescriptorLayoutCache::Init(VkDevice newDevice)
	{
		m_Device = newDevice;
	}

	VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info)
	{
		DescriptorLayoutInfo layoutinfo;
		layoutinfo.Bindings.reserve(info->bindingCount);
		bool isSorted = true;
		int32_t lastBinding = -1;
		for (uint32_t i = 0; i < info->bindingCount; i++) {
			layoutinfo.Bindings.push_back(info->pBindings[i]);

			//check that the bindings are in strict increasing order
			if (static_cast<int32_t>(info->pBindings[i].binding) > lastBinding)
			{
				lastBinding = info->pBindings[i].binding;
			}
			else {
				isSorted = false;
			}
		}
		if (!isSorted)
		{
			std::sort(layoutinfo.Bindings.begin(), layoutinfo.Bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
				return a.binding < b.binding;
				});
		}

		auto it = m_LayoutCache.find(layoutinfo);
		if (it != m_LayoutCache.end())
		{
			return (*it).second;
		}
		else {
			VkDescriptorSetLayout layout;
			CheckVkResult(vkCreateDescriptorSetLayout(m_Device, info, nullptr, &layout));

			//m_LayoutCache.emplace()
			//add to cache
			m_LayoutCache[layoutinfo] = layout;
			return layout;
		}
	}


	void DescriptorLayoutCache::Cleanup()
	{
		//delete every descriptor layout held
		for (auto pair : m_LayoutCache)
		{
			vkDestroyDescriptorSetLayout(m_Device, pair.second, nullptr);
		}
	}

	Hog::DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
	{
		DescriptorBuilder builder;

		builder.m_Cache = layoutCache;
		builder.m_Alloc = allocator;
		return builder;
	}


	Hog::DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding{};

		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;
		newBinding.binding = binding;

		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pBufferInfo = bufferInfo;
		newWrite.dstBinding = binding;

		m_Writes.push_back(newWrite);
		return *this;
	}


	Hog::DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding{};

		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;
		newBinding.binding = binding;

		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite{};
		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;

		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pImageInfo = imageInfo;
		newWrite.dstBinding = binding;

		m_Writes.push_back(newWrite);
		return *this;
	}

	bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
	{
		//build layout first
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;

		layoutInfo.pBindings = m_Bindings.data();
		layoutInfo.bindingCount = static_cast<uint32_t>(m_Bindings.size());

		layout = m_Cache->CreateDescriptorLayout(&layoutInfo);


		//Allocate descriptor
		bool success = m_Alloc->Allocate(&set, layout);
		if (!success) { return false; };

		//write descriptor

		for (VkWriteDescriptorSet& w : m_Writes) {
			w.dstSet = set;
		}

		vkUpdateDescriptorSets(m_Alloc->Device, static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);

		return true;
	}


	bool DescriptorBuilder::Build(VkDescriptorSet& set)
	{
		VkDescriptorSetLayout layout;
		return Build(set, layout);
	}


	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
	{
		if (other.Bindings.size() != Bindings.size())
		{
			return false;
		}
		else {
			//compare each of the bindings is the same. Bindings are sorted so they will match
			for (int i = 0; i < Bindings.size(); i++) {
				if (other.Bindings[i].binding != Bindings[i].binding)
				{
					return false;
				}
				if (other.Bindings[i].descriptorType != Bindings[i].descriptorType)
				{
					return false;
				}
				if (other.Bindings[i].descriptorCount != Bindings[i].descriptorCount)
				{
					return false;
				}
				if (other.Bindings[i].stageFlags != Bindings[i].stageFlags)
				{
					return false;
				}
			}
			return true;
		}
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
	{
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(Bindings.size());

		for (const VkDescriptorSetLayoutBinding& b : Bindings)
		{
			//pack the binding data into a single int64. Not fully correct but its ok
			size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

			//shuffle the packed binding data and xor it with the main hash
			result ^= hash<size_t>()(binding_hash);
		}

		return result;
	}

}
