#include "vkcpch.h"

#include "Material.h"
#include <VulkanCore/Renderer/GraphicsContext.h>

namespace VulkanCore
{
	static auto& context = GraphicsContext::Get();

	Ref<Material> Material::Create(Ref<Shader> shader, Ref<GraphicsPipeline> pipeline)
	{
		VKC_PROFILE_FUNCTION()

		Ref<Shader> shaderRef;
		Ref<GraphicsPipeline> pipelineRef;

		if (shader)
			shaderRef = shader;
		else
			shaderRef = ShaderLibrary::LoadOrGet("assets/shaders/Basic.glsl");

		if (pipeline)
			pipelineRef = pipeline;
		else
		{
			pipelineRef = shaderRef->CreateOrGetDefaultPipeline();
		}

		return CreateRef<Material>(shaderRef, pipelineRef);
	}

	void Material::Bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr)
	{
		VKC_PROFILE_FUNCTION()
	}

	static std::unordered_map<std::string, Ref<Material>> s_Materials;

	void MaterialLibrary::Add(const std::string& name, const Ref<Material>& material)
	{
		VKC_CORE_ASSERT(!Exists(name), "Shader already exists!");
		s_Materials[name] = material;
	}

	Ref<Material> MaterialLibrary::Create(const std::string& name, Ref<Shader> shader, Ref<GraphicsPipeline> pipeline)
	{
		Ref<Material> mat = Material::Create(shader, pipeline);
		s_Materials[name] = mat;
		return mat;
	}

	Ref<Material> MaterialLibrary::CreateOrGet(const std::string& name, Ref<Shader> shader,
		Ref<GraphicsPipeline> pipeline)
	{
		if (Exists(name))
			return Get(name);
		else
			return Create(name, shader, pipeline);
	}

	Ref<Material> MaterialLibrary::Get(const std::string& name)
	{
		VKC_CORE_ASSERT(Exists(name), "Material not found!");
		return s_Materials[name];
	}

	void MaterialLibrary::Deinitialize()
	{
		s_Materials.clear();
	}

	bool MaterialLibrary::Exists(const std::string& name)
	{
		return s_Materials.find(name) != s_Materials.end();
	}
}
