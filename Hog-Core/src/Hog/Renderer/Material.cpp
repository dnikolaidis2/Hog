#include "hgpch.h"

#include "Material.h"
#include <Hog/Renderer/GraphicsContext.h>

namespace Hog
{
	static auto& context = GraphicsContext::Get();

	Ref<Material> Material::Create(const std::string& name, MaterialData& data)
	{
		HG_PROFILE_FUNCTION()

		Ref<Shader> shaderRef = Shader::Create("Basic", "Basic.vertex", "Basic.fragment");
		return CreateRef<Material>(name, shaderRef, data);
	}

	void Material::Bind(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSetPtr)
	{
		HG_PROFILE_FUNCTION()
	}

	static std::unordered_map<std::string, Ref<Material>> s_Materials;

	void MaterialLibrary::Add(const std::string& name, const Ref<Material>& material)
	{
		HG_CORE_ASSERT(!Exists(name), "Shader already exists!");
		material->SetGPUIndex((int32_t)s_Materials.size());
		s_Materials[name] = material;
	}

	Ref<Material> MaterialLibrary::Create(const std::string& name, MaterialData& data)
	{
		Ref<Material> mat = Material::Create(name, data);
		Add(name, mat);
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
		HG_CORE_ASSERT(Exists(name), "Material not found!");
		return s_Materials[name];
	}

	std::array<MaterialGPUData, MATERIAL_ARRAY_SIZE> MaterialLibrary::GetGPUArray()
	{
		std::array<MaterialGPUData, MATERIAL_ARRAY_SIZE> arr;
		for (const auto &[name, mat] : s_Materials)
		{
			arr[mat->GetGPUIndex()] = MaterialGPUData(mat->GetMaterialData());
		}

		return arr;
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
