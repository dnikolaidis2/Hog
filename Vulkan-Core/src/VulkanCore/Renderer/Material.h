#pragma once

#include <VulkanCore/Renderer/Shader.h>
#include <VulkanCore/Renderer/GraphicsPipeline.h>

namespace VulkanCore
{
	class Material
	{
	public:
		static Ref<Material> Create(Ref<Shader> shader, Ref<GraphicsPipeline> pipeline);

		Material(Ref<Shader> shader, Ref<GraphicsPipeline> pipeline)
			: m_Shader(shader), m_Pipeline(pipeline)
		{}

		~Material() = default;

		Ref<Shader> GetShader() const { return m_Shader; }
		Ref<GraphicsPipeline> GetPipeline() const { return m_Pipeline; }

		void Bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr);
	private:
		Ref<Shader> m_Shader;
		Ref<GraphicsPipeline> m_Pipeline;
	};
}