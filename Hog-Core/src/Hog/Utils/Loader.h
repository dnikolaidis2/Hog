#pragma once

#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Texture.h"
#include "Hog/Renderer/Material.h"
#include "Hog/Renderer/Light.h"
#include "Hog/Renderer/EditorCamera.h"
#include "Hog/Debug/Instrumentor.h"
#include "Hog/Math/Math.h"

namespace Hog
{
	static bool LoadGltfFile(const std::string& filepath,
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

		tinygltf::Model gltfModel;
		tinygltf::TinyGLTF gltfContext;

		std::string error, warning;

		bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filepath);

		if (fileLoaded) 
		{
			// Extract name from filepath
			const auto path = std::filesystem::path(filepath);
			const auto currentPath = std::filesystem::current_path();

			// pushd
			std::filesystem::current_path(currentPath / path.parent_path());

			// Load images
			std::vector<Ref<Image>> images(gltfModel.images.size());
			std::transform(gltfModel.images.begin(), gltfModel.images.end(), images.begin(), [](tinygltf::Image& gltfImage) {
				return Image::LoadFromFile(gltfImage.uri);
			});

			// Load textures
			size_t textureInitialSize = textures.size();
			textures.resize(textureInitialSize + gltfModel.textures.size());
			for(size_t i = textureInitialSize; i < textures.size(); i++)
			{
				const auto& texture = gltfModel.textures[i];
				const auto& sampler = gltfModel.samplers[texture.sampler];
				
				SamplerType type{};
				switch (sampler.magFilter)
				{
					case TINYGLTF_TEXTURE_FILTER_NEAREST: type.MagFilter = VK_FILTER_NEAREST; break;
					case TINYGLTF_TEXTURE_FILTER_LINEAR: type.MagFilter = VK_FILTER_LINEAR; break;
				}

				switch (sampler.minFilter)
				{
					case TINYGLTF_TEXTURE_FILTER_NEAREST: type.MinFilter = VK_FILTER_NEAREST; break;
					case TINYGLTF_TEXTURE_FILTER_LINEAR: type.MinFilter = VK_FILTER_LINEAR; break;
					case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: type.MinFilter = VK_FILTER_NEAREST; break;
					case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: type.MinFilter = VK_FILTER_LINEAR; break;
					case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: type.MinFilter = VK_FILTER_NEAREST; break;
					case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: type.MinFilter = VK_FILTER_LINEAR; break;
				}

				switch (sampler.minFilter)
				{
					case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: type.MipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; break;
					case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: type.MipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; break;
					case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: type.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; break;
					case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: type.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; break;
				}

				switch (sampler.wrapS)
				{
					case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: type.AddressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
					case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: type.AddressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
					case TINYGLTF_TEXTURE_WRAP_REPEAT: type.AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
				}

				switch (sampler.wrapT)
				{
					case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: type.AddressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
					case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: type.AddressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
					case TINYGLTF_TEXTURE_WRAP_REPEAT: type.AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
				}
			
				textures[i] = Texture::Create(type, images[texture.source]);
				textures[i]->SetGPUIndex(textureInitialSize + i);
			}

			// Load materials
			materialBuffer = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(MaterialGPUData) * gltfModel.materials.size());
			materials.resize(gltfModel.materials.size());
			size_t materialOffset = 0;
			for (size_t i = 0; i < gltfModel.materials.size(); i++)
			{
				const auto& material = gltfModel.materials[i];
				MaterialData matData{};

				matData.DiffuseColor = glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());

				if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
				{
					matData.DiffuseTexture = textures[textureInitialSize + material.pbrMetallicRoughness.baseColorTexture.index];
				}

				materials[i] = Material::Create(material.name, matData);
				materials[i]->SetGPUIndex(i);
				materials[i]->UpdateData(materialBuffer, materialOffset);
				materialOffset += sizeof(MaterialGPUData);
			}

			// Create ligth buffer
			lightBuffer = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(LightData) * gltfModel.lights.size());
			size_t lightOffset = 0;

			// Load nodes
			for (size_t i = 0; i < gltfModel.nodes.size(); i++)
			{
				const auto& node = gltfModel.nodes[i];

				// Generate local node matrix
				glm::vec3 translation{ 0.0f };
				if (node.translation.size() == 3) {
					translation = glm::make_vec3(node.translation.data());
				}

				glm::quat rotation{1.0f, 1.0f, 1.0f, 1.0f};
				if (node.rotation.size() == 4) {
					rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
				}

				glm::vec3 scale{ 1.0f };
				if (node.scale.size() == 3) {
					scale = glm::make_vec3(node.scale.data());
				}

				glm::mat4 modelMat{ 1.0f };
				if (node.matrix.size() == 16) {
					modelMat = glm::make_mat4x4(node.matrix.data());
				}
				else
				{
					modelMat = glm::translate(glm::mat4(1.0f), translation)
						* glm::toMat4(rotation)
						* glm::scale(glm::mat4(1.0f), scale);
				}

				if (node.mesh != -1)
				{
					const tinygltf::Mesh mesh = gltfModel.meshes[node.mesh];
					
					auto nodeMesh = Mesh::Create(node.name);

					for (size_t j = 0; j < mesh.primitives.size(); j++) {
						const tinygltf::Primitive& primitive = mesh.primitives[j];
						const tinygltf::Material& material = gltfModel.materials[mesh.primitives[j].material];
						if (primitive.indices < 0) {
							continue;
						}

						if (material.alphaMode == "OPAQUE")
						{
							opaque.push_back(nodeMesh);
						}
						else
						{
							transparent.push_back(nodeMesh);
						}

						std::vector<Vertex> vertexData;

						// Vertices
						{
							const float* bufferPos = nullptr;
							const float* bufferNormals = nullptr;
							const float* bufferTexCoords = nullptr;
							const float* bufferColors = nullptr;
							const float* bufferTangents = nullptr;
							uint32_t numColorComponents;

							// Position attribute is required
							assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

							const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
							const tinygltf::BufferView& posView = gltfModel.bufferViews[posAccessor.bufferView];
							bufferPos = reinterpret_cast<const float*>(&(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));

							if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
								const tinygltf::Accessor& normAccessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
								const tinygltf::BufferView& normView = gltfModel.bufferViews[normAccessor.bufferView];
								bufferNormals = reinterpret_cast<const float*>(&(gltfModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
							}

							if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
								const tinygltf::Accessor& uvAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
								const tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
								bufferTexCoords = reinterpret_cast<const float*>(&(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
							}

							if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
							{
								const tinygltf::Accessor& colorAccessor = gltfModel.accessors[primitive.attributes.find("COLOR_0")->second];
								const tinygltf::BufferView& colorView = gltfModel.bufferViews[colorAccessor.bufferView];
								// Color buffer are either of type vec3 or vec4
								numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
								bufferColors = reinterpret_cast<const float*>(&(gltfModel.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
							}

							if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
							{
								const tinygltf::Accessor& tangentAccessor = gltfModel.accessors[primitive.attributes.find("TANGENT")->second];
								const tinygltf::BufferView& tangentView = gltfModel.bufferViews[tangentAccessor.bufferView];
								bufferTangents = reinterpret_cast<const float*>(&(gltfModel.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
							}

							for (size_t v = 0; v < posAccessor.count; v++) {
								Vertex vertex{};
								vertex.Position = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
								vertex.Normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
								vertex.TexCoords = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
								/*if (bufferColors) {
									switch (numColorComponents) {
									case 3:
										vert.Color = glm::vec4(glm::make_vec3(&bufferColors[v * 3]), 1.0f);
									case 4:
										vert.Color = glm::make_vec4(&bufferColors[v * 4]);
									}
								}
								else {
									vert.color = glm::vec4(1.0f);
								}*/
								vertex.Tangent = bufferTangents ? glm::vec4(glm::make_vec4(&bufferTangents[v * 4])) : glm::vec4(0.0f);
								vertex.MaterialIndex = materials[mesh.primitives[j].material]->GetGPUIndex();
								vertexData.push_back(vertex);
							}
						}

						std::vector<uint32_t> indexData;

						// Indices
						{
							const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
							const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
							const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];


							switch (accessor.componentType) {
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
								uint32_t* buf = new uint32_t[accessor.count];
								memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
								for (size_t index = 0; index < accessor.count; index++) {
									indexData.push_back(buf[index]);
								}
								delete[] buf;
								break;
							}
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
								uint16_t* buf = new uint16_t[accessor.count];
								memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
								for (size_t index = 0; index < accessor.count; index++) {
									indexData.push_back(buf[index]);
								}
								delete[] buf;
								break;
							}
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
								uint8_t* buf = new uint8_t[accessor.count];
								memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
								for (size_t index = 0; index < accessor.count; index++) {
									indexData.push_back(buf[index]);
								}
								delete[] buf;
								break;
							}
							default:
								HG_CORE_ERROR("Index component type {0} not supported!", accessor.componentType);
								return false;
							}
						}

						nodeMesh->AddPrimitive(vertexData, indexData);
						nodeMesh->Build();
						nodeMesh->SetModelMatrix(modelMat);
					}
				}
				else if(node.camera != -1)
				{
					const tinygltf::Camera& camera = gltfModel.cameras[node.camera];
					glm::mat4 projection = glm::perspective(
						camera.perspective.yfov,
						camera.perspective.aspectRatio,
						camera.perspective.znear,
						camera.perspective.zfar);
					glm::mat4 view = glm::translate(glm::mat4(1.0f), translation)
						* glm::toMat4(rotation)
						* glm::rotate(glm::mat4(1.f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					glm::mat4 cameraMat = projection * glm::inverse(view);

					cameras[camera.name] = cameraMat;
				}
				else if (node.extensions.size())
				{
					if (node.extensions.find("KHR_lights_punctual") != node.extensions.end())
					{
						if (node.extensions.at("KHR_lights_punctual").IsObject())
						{
							const tinygltf::Light light = gltfModel.lights[node.extensions.at("KHR_lights_punctual").Get("light").GetNumberAsInt()];
							LightType type;
							if (light.type == "directional")
							{
								type = LightType::Directional; break;
								/*case cgltf_light_type_spot: type = LightType::Spot; break;
								case cgltf_light_type_point: type = LightType::Point; break;*/
							}

							glm::vec4 color{ 1.0f };
							if (light.color.size() == 4)
							{
								color = glm::make_vec4(light.color.data());
							}
							else
							{
								color = glm::vec4(glm::make_vec3(light.color.data()), 1.0f);
							}

							auto lightRef = Light::Create({
								.Position = {translation},
								.Type = type,
								.Color =  color,
								.Direction = glm::eulerAngles(rotation),
								.Intensity = static_cast<float>(light.intensity),
							});

							lights.push_back(lightRef);
							lightRef->UpdateData(lightBuffer, lightOffset);
							lightOffset += sizeof(LightData);
						}
					}
				}
			}

			// popd
			std::filesystem::current_path(currentPath);
			return true;
		}

		HG_CORE_ERROR("{0}", warning);
		HG_CORE_ERROR("{0}", error);
		return false;
	}
}