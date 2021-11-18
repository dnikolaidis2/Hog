#pragma once

#include "VulkanCore.h"
#include <vk_mem_alloc.h>

class SandboxLayer : public VulkanCore::Layer
{
public:
	SandboxLayer();
	virtual ~SandboxLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(VulkanCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(VulkanCore::Event& e) override;
	bool OnResized(VulkanCore::FrameBufferResizeEvent& e);
private:
	VulkanCore::Ref<VulkanCore::Shader> m_Shader;

	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

	VkBuffer m_VertexBuffer;
	VmaAllocation m_VertexBufferAllocation;

	VkBuffer m_IndexBuffer;
	VmaAllocation m_IndexBufferAllocation;

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VmaAllocation> m_UniformBuffersAllocations;

	const std::vector<VulkanCore::Vertex> m_Vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> m_Indices = {
		0, 1, 2, 2, 3, 0
	};
};
