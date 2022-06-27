#pragma once

#include <cgltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Texture.h"
#include "Hog/Renderer/Material.h"
#include "Hog/Renderer/Light.h"
#include "Hog/Renderer/EditorCamera.h"
#include "Hog/Debug/Instrumentor.h"
#include "Hog/Math/Math.h"

namespace Hog
{
	struct LoaderOptions
	{
		bool SwapFrontFace = false;
		bool FlipYPosition = false;
	};

	static bool LoadGltfFile(const std::string& filepath,
		LoaderOptions options,
		std::vector<Ref<Mesh>>& opaque,
		std::vector<Ref<Mesh>>& transparent,
		std::unordered_map<std::string, glm::mat4>& cameras,
		std::vector<Ref<Texture>>& textures,
		std::vector<Ref<Material>>& materials,
		Ref<Buffer>& materialBuffer,
		std::vector<Ref<Light>>& lights,
		Ref<Buffer>& lightBuffer)
	{
		HG_PROFILE_FUNCTION();

		cgltf_options cgltfOptions = {};
		cgltf_data* data = NULL;
		cgltf_result result = cgltf_parse_file(&cgltfOptions, filepath.c_str(), &data);
		if (result == cgltf_result_success)
		{
			// Extract name from filepath
			auto path = std::filesystem::path(filepath);
			auto currentPath = std::filesystem::current_path();
			std::filesystem::current_path(currentPath / path.parent_path());

			for (int i = 0; i < data->buffers_count; ++i)
			{
				result = cgltf_load_buffers(&cgltfOptions, data, data->buffers[i].uri);
				if (result != cgltf_result_success)
				{
					cgltf_free(data);
					return false;
				}
			}

			std::vector<Ref<Image>> images(data->images_count);

			for (int i = 0; i < data->images_count; i++)
			{
				images[i] = Image::LoadFromFile(data->images[i].uri);
			}

			auto initialSize = textures.size();
			for (int i = 0; i < data->textures_count; i++)
			{
				auto * texture = &(data->textures[i]);
				SamplerType type {};
				
				switch (texture->sampler->mag_filter)
				{
					case 9728: type.MagFilter = VK_FILTER_NEAREST; break;
					case 9729: type.MagFilter = VK_FILTER_LINEAR; break;
				}

				switch (texture->sampler->min_filter)
				{
					case 9728: type.MinFilter = VK_FILTER_NEAREST; break;
					case 9729: type.MinFilter = VK_FILTER_LINEAR; break;
					case 9984: type.MinFilter = VK_FILTER_NEAREST; break;
					case 9985: type.MinFilter = VK_FILTER_LINEAR; break;
					case 9986: type.MinFilter = VK_FILTER_NEAREST; break;
					case 9987: type.MinFilter = VK_FILTER_LINEAR; break;
				}

				switch (texture->sampler->min_filter)
				{
					case 9984: type.MipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; break;
					case 9985: type.MipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; break;
					case 9986: type.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; break;
					case 9987: type.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; break;
				}

				switch (texture->sampler->wrap_s)
				{
					case 33071: type.AddressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
					case 33648: type.AddressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
					case 10497: type.AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
				}

				switch (texture->sampler->wrap_t)
				{
					case 33071: type.AddressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
					case 33648: type.AddressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
					case 10497: type.AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
				}

				Ref<Texture> textureRef = Texture::Create(type, images[texture->image - data->images]);
				textureRef->SetGPUIndex(initialSize + i);

				textures.push_back(textureRef);
			}

			materialBuffer = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(MaterialGPUData) * data->materials_count);
			size_t offset = 0;

			for (int i = 0; i < data->materials_count; i++)
			{
 				const auto material = &(data->materials[i]);
				MaterialData matData {};

				matData.DiffuseColor = glm::vec4(material->pbr_metallic_roughness.base_color_factor[0],
					material->pbr_metallic_roughness.base_color_factor[1],
					material->pbr_metallic_roughness.base_color_factor[2],
					material->pbr_metallic_roughness.base_color_factor[3]);

				if (material->pbr_metallic_roughness.base_color_texture.texture)
				{
					matData.DiffuseTexture = textures[material->pbr_metallic_roughness.base_color_texture.texture - data->textures];
				}

				if (material->normal_texture.texture)
				{
					matData.BumpMap = textures[material->normal_texture.texture - data->textures];
				}

				materials.push_back(Material::Create(material->name, matData));
				materials[i]->SetGPUIndex(i);
				materials[i]->UpdateData(materialBuffer, offset);
				offset += sizeof(MaterialGPUData);
			}
			
			lightBuffer = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(LightData) * data->lights_count);
			size_t lightOffset = 0;

			for (int i = 0; i < data->nodes_count; ++i)
			{
				const auto node = &(data->nodes[i]);

				// Load model matrix
				glm::mat4 modelMat{ 1.0f };
				glm::vec3 translation{ 1.0f };
				glm::quat rotation {};
				glm::vec3 scale{ 1.0f };
				if (node->has_matrix)
				{
					modelMat = glm::mat4(
						node->matrix[0], node->matrix[1], node->matrix[2], node->matrix[3],
						node->matrix[4], node->matrix[5], node->matrix[6], node->matrix[7],
						node->matrix[8], node->matrix[9], node->matrix[10], node->matrix[11],
						node->matrix[12], node->matrix[13], node->matrix[14], node->matrix[15]
					);
				}
				else
				{
					if (node->has_translation)
					{
						translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
						modelMat = glm::translate(modelMat, translation);
					}

					if (node->has_rotation)
					{
						rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
						modelMat *= glm::toMat4(rotation);
					}

					if (node->has_scale)
					{
						scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
						modelMat = glm::scale(modelMat, scale);
					}
				}

				if (node->mesh)
				{
					const auto mesh = node->mesh;
					for (int j = 0; j < mesh->primitives_count; ++j)
					{
						const auto primitive = &(mesh->primitives[j]);

						auto nodeMesh = Mesh::Create(node->name);
						if (primitive->material->alpha_mode == cgltf_alpha_mode_opaque)
						{
							opaque.push_back(nodeMesh);
						}
						else
						{
							transparent.push_back(nodeMesh);
						}

						std::vector<uint16_t> indexData;
						std::vector<Vertex> vertexData;

						indexData.resize(primitive->indices->count);
						for (int z = 0; z < primitive->indices->count; z += 3)
						{
							if (options.SwapFrontFace)
							{
								indexData[z + 2] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z));
								indexData[z + 1] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z + 1));
								indexData[z + 0] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z + 2));
							}
							else
							{
								indexData[z + 0] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z));
								indexData[z + 1] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z + 1));
								indexData[z + 2] = static_cast<uint16_t>(cgltf_accessor_read_index(primitive->indices, z + 2));
							}
						}

						vertexData.resize(primitive->attributes->data->count);
						std::vector<glm::vec3> positions;
						std::vector<glm::vec3> normals;
						std::vector<glm::vec2> texcoords;
						std::vector<glm::vec4> tangent;
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
							case cgltf_attribute_type_tangent:
							{
								cgltf_size count = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

								tangent.resize(count / 4);

								cgltf_accessor_unpack_floats(attribute->data, reinterpret_cast<cgltf_float*>(tangent.data()), count);
							}break;
							default: break;
							}
						}

						for (int z = 0; z < vertexData.size(); ++z)
						{
							vertexData[z].Position = positions[z];
							vertexData[z].Normal = normals[z];
							vertexData[z].TexCoords = texcoords[z];
							vertexData[z].Tangent = tangent[z];
							
							if (primitive->material)
							{
								vertexData[z].MaterialIndex = materials[primitive->material - data->materials]->GetGPUIndex();
							}
						}

						nodeMesh->AddPrimitive(vertexData, indexData);
						nodeMesh->Build();
						nodeMesh->SetModelMatrix(modelMat);
					}
				}

				if (node->camera)
				{
					glm::mat4 projection = glm::perspective(
						node->camera->data.perspective.yfov,
						node->camera->data.perspective.aspect_ratio,
						node->camera->data.perspective.znear,
						node->camera->data.perspective.zfar);
					glm::mat4 view = glm::translate(glm::mat4(1.0f), translation)
						* glm::toMat4(rotation);
					glm::mat4 camera = projection * glm::inverse(view);

					cameras[node->camera->name] = camera;

					/*
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

					objects.push_back(mesh);
					*/
				}

				if (node->light)
				{
					LightType type;
					switch (node->light->type)
					{
						case cgltf_light_type_directional: type = LightType::Directional; break;
						case cgltf_light_type_spot: type = LightType::Spot; break;
						case cgltf_light_type_point: type = LightType::Point; break;
					}

					auto light = Light::Create({
						.Position = {translation},
						.Type = type,
						.Color = {node->light->color[0], node->light->color[1], node->light->color[2], 1.0f},
						.Direction = glm::vec3(0.0f, 1.0f, 0.0f) * rotation,
						.Intensity = node->light->intensity,
					});
					
					lights.push_back(light);
					light->UpdateData(lightBuffer, lightOffset);
					lightOffset += sizeof(LightData);
				}
			}

			std::filesystem::current_path(currentPath);
			cgltf_free(data);
			return true;
		}

		return false;
	}
}