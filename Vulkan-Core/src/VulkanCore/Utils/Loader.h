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

		std::filesystem::current_path(currentPath);

		//if we have any error, print it to the console, and break the mesh loading.
		//This happens if the file can't be found or is malformed
		if (!errors.empty()) {
			VKC_CORE_ERROR(errors);
			if (!ret)
				return ret;
		}

		for (size_t s = 0; s < objMaterials.size(); s++)
		{
			MaterialLibrary::Create(objMaterials[s].name, nullptr, nullptr);
		}
		
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


					verties.push_back({ {vx, vy, vz}, {nx, ny, nz},{tx, ty}, {nx, ny, nz} });
				}

				indexOffset += fv;
			}

			mesh->Load();
			objects[s] = RendererObject::Create(std::move(mesh), MaterialLibrary::Get(objMaterials[shapes[s].mesh.material_ids[0]].name));
		}

		return ret;
	}
}