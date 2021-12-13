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
		std::string m_Name;
		Ref<Shader> m_Shader;
		Ref<GraphicsPipeline> m_Pipeline;
	};

	class MaterialLibrary
	{
	public:
		MaterialLibrary() = delete;

		static void Add(const std::string& name, const Ref<Material>& shader);
		static void Add(const Ref<Material>& shader);
		static Ref<Material> Create(const std::string& name, Ref<Shader> shader, Ref<GraphicsPipeline> pipeline);
		static Ref<Material> CreateOrGet(const std::string& name, Ref<Shader> shader, Ref<GraphicsPipeline> pipeline);

		static Ref<Material> Get(const std::string& name);

		static void Deinitialize();

		static bool Exists(const std::string& name);
	};
}