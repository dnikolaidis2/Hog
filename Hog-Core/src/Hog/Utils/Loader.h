#pragma once

#include "Hog/Renderer/Mesh.h"
#include "Hog/Renderer/Texture.h"
#include "Hog/Renderer/Material.h"
#include "Hog/Renderer/Light.h"
#include "Hog/Renderer/Camera.h"

namespace Hog
{
	namespace Util
	{
		class Loader
		{
		public:
			struct Options
			{
				bool SwapFrontFace = false;
				bool FlipYPosition = false;
			};

		public:
			static bool LoadGltf(const std::string& filepath,
				Options options,
				std::vector<Ref<Mesh>>& opaque,
				std::vector<Ref<Mesh>>& transparent,
				std::unordered_map<std::string, Camera>& cameras,
				std::vector<Ref<Texture>>& textures,
				std::vector<Ref<Material>>& materials,
				Ref<Buffer>& materialBuffer,
				std::vector<Ref<Light>>& lights,
				Ref<Buffer>& lightBuffer);
		};
	}
}