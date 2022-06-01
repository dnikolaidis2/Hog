#pragma once

#include <tiny_obj_loader.h>
#include <cgltf.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Material.h"
#include "Hog/Renderer/EditorCamera.h"
#include "Hog/Debug/Instrumentor.h"
#include "Hog/Math/Math.h"

namespace Hog
{
	/*inline static bool LoadObjFile(const std::string& filepath, std::vector<Ref<RendererObject>>& objects)
	{
		HG_PROFILE_FUNCTION();

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
			HG_CORE_ERROR(errors);
			if (!ret)
				return ret;
		}

		std::vector<Ref<Material>> mat(objMaterials.size());
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
			else
			{
				data.DiffuseTexture = nullptr;
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

			if (MaterialLibrary::Exists(objMaterials[s].name))
			{
				std::string matName = objMaterials[s].name + ".001";
				mat[s] = MaterialLibrary::Create(matName, data);
			}
			else
			{
				mat[s] = MaterialLibrary::Create(objMaterials[s].name, data);
			}
		}

		std::filesystem::current_path(currentPath);

		size_t initialSize = objects.size();
		objects.resize(initialSize + shapes.size());
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

					tinyobj::real_t nx = 0.0f;
					tinyobj::real_t ny = 0.0f;
					tinyobj::real_t nz = 0.0f;

					if (idx.normal_index != -1)
					{
						nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
						ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
						nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
					}

					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

					verties.push_back({ {vx, vy, vz}, {nx, ny, nz}, {tx, ty}, mat[shapes[s].mesh.material_ids[f]]->GetGPUIndex()});
				}

				indexOffset += fv;
			}

			mesh->Load();
			objects[initialSize + s] = RendererObject::Create(std::move(mesh), mat[shapes[s].mesh.material_ids[0]]);
		}

		return ret;
	}*/

	inline static bool LoadGltfFile(const std::string& filepath, std::vector<Ref<Mesh>>& objects, std::vector<glm::mat4>& cameras)
	{
		HG_PROFILE_FUNCTION();

		cgltf_options options = {};
		cgltf_data* data = NULL;
		cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
		if (result == cgltf_result_success)
		{
			// Extract name from filepath
			auto path = std::filesystem::path(filepath);
			auto currentPath = std::filesystem::current_path();
			std::filesystem::current_path(currentPath / path.parent_path());

			for (int i = 0; i < data->buffers_count; ++i)
			{
				result = cgltf_load_buffers(&options, data, data->buffers[i].uri);
				if (result != cgltf_result_success)
				{
					cgltf_free(data);
					return false;
				}
			}

			for (int i = 0; i < data->images_count; i++)
			{
				TextureLibrary::LoadOrGet(data->images[i].uri);
			}

			for (int i = 0; i < data->materials_count; i++)
			{
				const auto material = &(data->materials[i]);
				MaterialData matData = {};

				matData.DiffuseColor = glm::vec4(material->pbr_metallic_roughness.base_color_factor[0],
					material->pbr_metallic_roughness.base_color_factor[1],
					material->pbr_metallic_roughness.base_color_factor[2],
					material->pbr_metallic_roughness.base_color_factor[3]);

				if (material->pbr_metallic_roughness.base_color_texture.texture)
				{
					matData.DiffuseTexture = TextureLibrary::LoadOrGet(material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
				}
				else
				{
					matData.DiffuseTexture = TextureLibrary::LoadOrGet("zeros");
				}

				MaterialLibrary::Create(material->name, matData);
			}

			for (int i = 0; i < data->nodes_count; ++i)
			{
				const auto node = &(data->nodes[i]);
				if (node->mesh)
				{
					auto nodeMesh = Mesh::Create(node->name);
					objects.push_back(nodeMesh);

					const auto mesh = node->mesh;
					for (int j = 0; j < mesh->primitives_count; ++j)
					{
						const auto primitive = &(mesh->primitives[j]);

						std::vector<uint16_t> indexData;
						std::vector<Vertex> vertexData;

						indexData.resize(primitive->indices->count);
						for (int z = 0; z < primitive->indices->count; z += 3)
						{
							indexData[z + 2] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z));
							indexData[z + 1] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z + 1));
							indexData[z + 0] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z + 2));
						}

						vertexData.resize(primitive->attributes->data->count);
						std::vector<glm::vec3> positions;
						std::vector<glm::vec3> normals;
						std::vector<glm::vec2> texcoords;
						for (int z = 0; z < primitive->attributes_count; ++z)
						{
							const auto attribute = &(primitive->attributes[z]);

							switch (attribute->type)
							{
							case cgltf_attribute_type_position:
							{
								cgltf_size count = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

								positions.resize(count / 3);

								cgltf_accessor_unpack_floats(attribute->data, reinterpret_cast<cgltf_float*>(positions.data()), count);
							}break;
							case cgltf_attribute_type_normal:
							{
								cgltf_size count = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

								normals.resize(count / 3);

								cgltf_accessor_unpack_floats(attribute->data, reinterpret_cast<cgltf_float*>(normals.data()), count);
							}break;
							case cgltf_attribute_type_texcoord:
							{
								cgltf_size count = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

								texcoords.resize(count / 2);

								cgltf_accessor_unpack_floats(attribute->data, reinterpret_cast<cgltf_float*>(texcoords.data()), count);
							}break;
							default: break;
							}
						}

						for (int z = 0; z < vertexData.size(); ++z)
						{
							vertexData[z].Position = positions[z];
							vertexData[z].Normal = normals[z];
							vertexData[z].TexCoords = texcoords[z];
							
							if (primitive->material)
							{
								vertexData[z].MaterialIndex = MaterialLibrary::Get(primitive->material->name)->GetGPUIndex();
							}
						}

						nodeMesh->AddPrimitive(vertexData, indexData);
					}

					nodeMesh->Build();

					glm::mat4 modelMat = glm::mat4(1.0f);
					if (node->has_matrix)
					{
						modelMat = glm::mat4(node->matrix[0], node->matrix[1], node->matrix[2], node->matrix[3],
							node->matrix[4], node->matrix[5], node->matrix[6], node->matrix[7],
							node->matrix[8], node->matrix[9], node->matrix[10], node->matrix[11],
							node->matrix[12], node->matrix[13], node->matrix[14], node->matrix[15]);
					}
					else
					{
						if (node->has_translation)
						{
							modelMat = glm::translate(modelMat, glm::vec3(node->translation[0], node->translation[1], node->translation[2]));
						}

						if (node->has_rotation)
						{
							modelMat *= glm::toMat4(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
						}

						if (node->has_scale)
						{
							modelMat = glm::scale(modelMat, glm::vec3(node->scale[0], node->scale[1], node->scale[2]));
						}
					}

					nodeMesh->SetModelMatrix(modelMat);
				}

				if (node->camera)
				{
					glm::mat4 projection = glm::perspective(
						node->camera->data.perspective.yfov,
						node->camera->data.perspective.aspect_ratio,
						node->camera->data.perspective.znear,
						node->camera->data.perspective.zfar);
					glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(node->parent->translation[0], node->parent->translation[1], node->parent->translation[2]))
						* glm::toMat4(
							glm::quat(node->parent->rotation[3], node->parent->rotation[0], node->parent->rotation[1], node->parent->rotation[2])
							* glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]))
						* glm::rotate(glm::mat4(1.f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
						;
					glm::mat4 camera = projection * glm::inverse(view);

					cameras.push_back(camera);

					std::vector<glm::vec3> frustrumCorners;
					Math::CalculateFrustrumCorners(frustrumCorners, projection);

					std::vector<uint16_t> indexData = {
						7, 6, 5,
						4, 7, 5,

						2, 1, 6,
						1, 5, 6,

						1, 0, 5,
						0, 4, 5,

						0, 3, 4,
						2, 7, 3, // FIX ME

						3, 2, 7,
						2, 6, 7,

						0, 1, 2,
						3, 0, 2,
					};

					std::vector<Vertex> vertexData;

					for (size_t i = 0; i < frustrumCorners.size(); i++)
					{
						Vertex vertex = {
							frustrumCorners[i]
						};

						vertexData.push_back(vertex);
					}

					auto mesh = Mesh::Create("Frustrum");
					mesh->AddPrimitive(vertexData, indexData);
					mesh->Build();
					mesh->SetModelMatrix(view);

					// objects.push_back(mesh);
				}
			}

			std::filesystem::current_path(currentPath);
			cgltf_free(data);
			return true;
		}

		return false;
	}
}