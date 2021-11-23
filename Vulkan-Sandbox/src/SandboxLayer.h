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
	Ref<Shader> m_Shader;

	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	Ref<GraphicsPipeline> m_Pipeline;

	Ref<VertexBuffer> m_VertexBuffer;
	Ref<IndexBuffer> m_IndexBuffer;

	std::vector<Ref<MemoryBuffer>> m_UniformBuffers;

	const std::vector<Vertex> m_Vertices = {
		{{-0.5f, -0.5f, 0.0f}, {}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> m_Indices = {
		0, 1, 2, 2, 3, 0
	};
};
