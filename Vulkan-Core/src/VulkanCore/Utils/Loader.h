#pragma once

#include "tiny_obj_loader.h"
#include <VulkanCore/Renderer/Mesh.h>

namespace VulkanCore
{
	static bool LoadObjFile(const std::string& filepath, std::vector<Mesh>& meshes)
	{
		//attrib will contain the vertex arrays of the file
		tinyobj::attrib_t attrib;
		//shapes contains the info for each separate object in the file
		std::vector<tinyobj::shape_t> shapes;
		//materials contains the information about the material of each shape, but we won't use it.
		std::vector<tinyobj::material_t> materials;

		//error and warning output from the load function
		std::string errors;

		// Extract name from filepath
		auto path = std::filesystem::path(filepath);
		auto currentPath = std::filesystem::current_path();
		std::filesystem::current_path(currentPath / path.parent_path());

		//load the OBJ file
		tinyobj::LoadObj(&attrib, &shapes, &materials, &errors, path.filename().string().c_str());

		std::filesystem::current_path(currentPath);

		//if we have any error, print it to the console, and break the mesh loading.
		//This happens if the file can't be found or is malformed
		if (!errors.empty()) {
			VKC_CORE_ERROR(errors);
			return false;
		}

		meshes.resize(meshes.size() + shapes.size());
		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++) {
			meshes[s].SetName(shapes[s].name);
			// Loop over faces(polygon)
			size_t indexOffset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				auto verties = meshes[s].GetVertices();
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

					verties->push_back({{vx, vy, vz}, {nx, ny, nz}, {nx, ny, nz}});
				}

				indexOffset += fv;
			}

			meshes[s].Create();
		}

		return true;
	}
}