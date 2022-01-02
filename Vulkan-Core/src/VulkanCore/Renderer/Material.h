#pragma once

#include <glm/glm.hpp>

#include "Constants.h"
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
		int32_t IlluminationModel = 0;
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

	struct MaterialGPUData
	{
		glm::vec3 AmbientColor = glm::vec3(0.0f);
		int32_t AmbientTexture = -1;

		glm::vec3 DiffuseColor = glm::vec3(0.0f);
		int32_t DiffuseTexture = -1;

		glm::vec3 SpecularColor = glm::vec3(0.0f);
		int32_t SpecularTexture = -1;

		int32_t SpecularHighlightTexture = -1;
		float Specularity = 0.0f;
		float IOR = 0.0f;
		float Dissolve = 0.0f;

		glm::vec3 EmissiveColor = glm::vec3(0.0f);
		int32_t AlphaMap = -1;

		glm::vec3 TransmittanceFilter = glm::vec3(0.0f);
		int32_t BumpMap = -1;

		int32_t DisplacementMap = -1;
		int32_t IlluminationModel = 0;
		uint8_t Padding[8];

		MaterialGPUData() = default;
		MaterialGPUData(const MaterialData& data)
			:	Specularity(data.Specularity),
				IOR(data.IOR),
				Dissolve(data.Dissolve),
				IlluminationModel(data.IlluminationModel),
				TransmittanceFilter(data.TransmittanceFilter),
				AmbientColor(data.AmbientColor),
				DiffuseColor(data.DiffuseColor),
				SpecularColor(data.SpecularColor),
				EmissiveColor(data.EmissiveColor)
		{
			if (data.AmbientTexture) 
				AmbientTexture = data.AmbientTexture->GetGPUIndex();
			if (data.DiffuseTexture) 
				DiffuseTexture = data.DiffuseTexture->GetGPUIndex();
			if (data.SpecularTexture) 
				SpecularTexture = data.SpecularTexture->GetGPUIndex();
			if (data.SpecularHighlightTexture) 
				SpecularHighlightTexture = data.SpecularHighlightTexture->GetGPUIndex();
			if (data.AlphaMap) 
				AlphaMap = data.AlphaMap->GetGPUIndex();
			if (data.BumpMap) 
				BumpMap = data.BumpMap->GetGPUIndex();
			if (data.DisplacementMap) 
				DisplacementMap = data.DisplacementMap->GetGPUIndex();
		}
		MaterialGPUData(const MaterialGPUData& data) = default;
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
		const MaterialData& GetMaterialData() const { return m_Data; }
		void SetGPUIndex(int32_t ind) { m_GPUIndex = ind; }
		int32_t GetGPUIndex() { return m_GPUIndex; }

		void Bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr);
	private:
		std::string m_Name;
		Ref<Shader> m_Shader;
		Ref<GraphicsPipeline> m_Pipeline;
		MaterialData m_Data;
		int32_t m_GPUIndex = -1;
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
		static std::array<MaterialGPUData, MATERIAL_ARRAY_SIZE> GetGPUArray();

		static void Deinitialize();

		static bool Exists(const std::string& name);
	};
}