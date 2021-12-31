#pragma once

#include <tiny_obj_loader.h>

#include "VulkanCore/Renderer/Mesh.h"
#include "VulkanCore/Renderer/RendererObject.h"
#include "VulkanCore/Renderer/Material.h"
#include "VulkanCore/Debug/Instrumentor.h"

namespace VulkanCore
{
	inline static bool LoadObjFile(const std::string& filepath, std::vector<Ref<RendererObject>>& objects)
	{
		VKC_PROFILE_FUNCTION();

		//attrib will contain the vertex arrays of the file
		tinyobj::attrib_t attrib;
		//shapes contains the info for each separate object in the file
		std::vector<tinyobj::shape_t> shapes;
		//materials contains the information about the material of each shape, but we won't use it.
		std::vector<tinyobj::material_t> objMaterials;

		//error and warning output from the load function
		std::string errors;

		// Extract name from filepath
		auto path = std::filesystem::path(filepath);
		auto currentPath = std::filesystem::current_path();
		std::filesystem::current_path(currentPath / path.parent_path());

		//load the OBJ file
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &objMaterials, &errors, path.filename().string().c_str());

		//if we have any error, print it to the console, and break the mesh loading.
		//This happens if the file can't be found or is malformed
		if (!errors.empty()) {
			VKC_CORE_ERROR(errors);
			if (!ret)
				return ret;
		}

		for (size_t s = 0; s < objMaterials.size(); s++)
		{
			MaterialData data = {};

			data.AmbientColor = glm::vec3(objMaterials[s].ambient[0], objMaterials[s].ambient[1], objMaterials[s].ambient[2]);
			data.DiffuseColor = glm::vec3(objMaterials[s].diffuse[0], objMaterials[s].diffuse[1], objMaterials[s].diffuse[2]);
			data.SpecularColor = glm::vec3(objMaterials[s].specular[0], objMaterials[s].specular[1], objMaterials[s].specular[2]);
			data.TransmittanceFilter = glm::vec3(objMaterials[s].transmittance[0], objMaterials[s].transmittance[1], objMaterials[s].transmittance[2]);
			data.EmissiveColor = glm::vec3(objMaterials[s].emission[0], objMaterials[s].emission[1], objMaterials[s].emission[2]);
			data.Specularity = objMaterials[s].shininess;
			data.IOR = objMaterials[s].ior;
			data.Dissolve = objMaterials[s].dissolve;
			data.IlluminationModel = objMaterials[s].illum;

			if (!objMaterials[s].ambient_texname.empty())
			{
				data.AmbientTexture = TextureLibrary::LoadOrGet(objMaterials[s].ambient_texname);
			}

			if (!objMaterials[s].diffuse_texname.empty())
			{
				data.DiffuseTexture = TextureLibrary::LoadOrGet(objMaterials[s].diffuse_texname);
			}

			if (!objMaterials[s].specular_texname.empty())
			{
				data.SpecularTexture = TextureLibrary::LoadOrGet(objMaterials[s].specular_texname);
			}

			if (!objMaterials[s].specular_highlight_texname.empty())
			{
				data.SpecularHighlightTexture = TextureLibrary::LoadOrGet(objMaterials[s].specular_highlight_texname);
			}

			if (!objMaterials[s].bump_texname.empty())
			{
				data.BumpMap = TextureLibrary::LoadOrGet(objMaterials[s].bump_texname);
			}

			if (!objMaterials[s].displacement_texname.empty())
			{
				data.DisplacementMap = TextureLibrary::LoadOrGet(objMaterials[s].displacement_texname);
			}

			if (!objMaterials[s].alpha_texname.empty())
			{
				data.AlphaMap = TextureLibrary::LoadOrGet(objMaterials[s].alpha_texname);
			}

			MaterialLibrary::Create(objMaterials[s].name, data);
		}

		std::filesystem::current_path(currentPath);

		objects.resize(objects.size() + shapes.size());
		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {
			auto mesh = Mesh::Create();
			mesh->SetName(shapes[s].name);
			// Loop over faces(polygon)
			size_t indexOffset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				auto& verties = mesh->GetVertices();
				//hardcode loading to triangles
				int fv = 3;

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];

					tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
					
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];


					verties.push_back({ {vx, vy, vz}, {nx, ny, nz}, {tx, ty}, {nx, ny, nz} });
				}

				indexOffset += fv;
			}

			mesh->Load();
			objects[s] = RendererObject::Create(std::move(mesh), MaterialLibrary::Get(objMaterials[shapes[s].mesh.material_ids[0]].name));
		}

		return ret;
	}
}