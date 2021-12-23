#include "vkcpch.h"

#include "Material.h"
#include <VulkanCore/Renderer/GraphicsContext.h>

namespace VulkanCore
{
	static auto& context = GraphicsContext::Get();

	Ref<Material> Material::Create(MaterialData& data)
	{
		VKC_PROFILE_FUNCTION()

		Ref<Shader> shaderRef = ShaderLibrary::LoadOrGet("assets/shaders/Basic.glsl");
		return CreateRef<Material>(shaderRef, shaderRef->CreateOrGetDefaultPipeline(), data);
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

	Ref<Material> MaterialLibrary::Create(const std::string& name, MaterialData& data)
	{
		Ref<Material> mat = Material::Create(data);
		s_Materials[name] = mat;
		return mat;
	}

	Ref<Material> MaterialLibrary::CreateOrGet(const std::string& name)
	{
		if (Exists(name))
			return Get(name);
		else
		{
			MaterialData data = {};
			return Create(name, data);
		}
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
