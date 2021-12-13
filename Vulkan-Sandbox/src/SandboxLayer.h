#pragma once

#include "VulkanCore.h"
#include <vk_mem_alloc.h>

using namespace VulkanCore;

class SandboxLayer : public Layer
{
public:
	SandboxLayer();
	virtual ~SandboxLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Event& e) override;
	bool OnResized(FrameBufferResizeEvent& e);
private:
	std::vector<Ref<RendererObject>> m_Objects;
	
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	std::vector<Ref<MemoryBuffer>> m_UniformBuffers;
	EditorCamera m_EditorCamera;

	uint64_t m_FrameNumber = 0;
};
