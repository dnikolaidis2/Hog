#pragma once

#include <glm/glm.hpp>

#include "VulkanCore/Renderer/Shader.h"
#include "VulkanCore/Renderer/GraphicsPipeline.h"
#include "VulkanCore/Renderer/Image.h"

namespace VulkanCore
{
	struct MaterialData
	{
		float Specularity = 0.0f;
		float IOR = 0.0f;
		float Dissolve = 0.0f;
		int IlluminationModel = 0;
		glm::vec3 TransmittanceFilter = glm::vec3(0.0f);
		glm::vec3 AmbientColor = glm::vec3(0.0f);
		glm::vec3 DiffuseColor = glm::vec3(0.0f);
		glm::vec3 SpecularColor = glm::vec3(0.0f);
		glm::vec3 EmissiveColor = glm::vec3(0.0f);
		
		Ref<Image> AmbientTexture;
		Ref<Image> DiffuseTexture;
		Ref<Image> SpecularTexture;
		Ref<Image> SpecularHighlightTexture;
		Ref<Image> AlphaMap;
		Ref<Image> BumpMap;
		Ref<Image> DisplacementMap;
	};

	class Material
	{
	public:
		static Ref<Material> Create(MaterialData& data);
	public:

		Material(const Ref<Shader>& shader, const Ref<GraphicsPipeline>& pipeline, MaterialData& data)
			: m_Shader(shader), m_Pipeline(pipeline), m_Data(data)
		{}

		~Material() = default;

		const Ref<Shader>& GetShader() const { return m_Shader; }
		const Ref<GraphicsPipeline>& GetPipeline() const { return m_Pipeline; }
		const Ref<Image>& GetDiffuseTexture() const { return m_Data.DiffuseTexture; }

		void Bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr);
	private:
		std::string m_Name;
		Ref<Shader> m_Shader;
		Ref<GraphicsPipeline> m_Pipeline;
		MaterialData m_Data;
	};

	class MaterialLibrary
	{
	public:
		MaterialLibrary() = delete;

		static void Add(const std::string& name, const Ref<Material>& shader);
		static void Add(const Ref<Material>& shader);
		static Ref<Material> Create(const std::string& name, MaterialData& data);
		static Ref<Material> CreateOrGet(const std::string& name);

		static Ref<Material> Get(const std::string& name);

		static void Deinitialize();

		static bool Exists(const std::string& name);
	};
}