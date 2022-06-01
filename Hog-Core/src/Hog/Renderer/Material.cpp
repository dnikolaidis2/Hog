#include "hgpch.h"

#include "Material.h"

#include "Hog/Core/CVars.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Buffer.h"

AutoCVar_Int CVar_MaterialArraySize("material.array.size", "Material array size", 128);

namespace Hog
{

	static auto& context = GraphicsContext::Get();

	Ref<Material> Material::Create(const std::string& name, MaterialData& data)
	{
		HG_PROFILE_FUNCTION()

		return CreateRef<Material>(name, data);
	}

	static std::unordered_map<std::string, Ref<Material>> s_Materials;
	static Ref<Buffer> s_Buffer;

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

	Ref<Buffer> MaterialLibrary::GetBuffer()
	{
		s_Buffer = Buffer::Create(BufferDescription::Defaults::UniformBuffer, CVar_MaterialArraySize.Get() * sizeof(MaterialGPUData));

		std::vector<MaterialGPUData> GPUData(s_Materials.size());

		int i = 0;
		for (auto & [name, material]: s_Materials)
		{
			GPUData[material->GetGPUIndex()] = MaterialGPUData(material->GetMaterialData());
			i++;
		}

		s_Buffer->WriteData(GPUData.data(), GPUData.size() * sizeof(MaterialGPUData));

		return s_Buffer;
	}

	void MaterialLibrary::Clneaup()
	{
		s_Materials.clear();
		s_Buffer.reset();
	}

	bool MaterialLibrary::Exists(const std::string& name)
	{
		return s_Materials.find(name) != s_Materials.end();
	}
}
