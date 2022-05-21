#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <shaderc/shaderc.h>

namespace Hog
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		int32_t MaterialIndex;
	};

	struct BufferDescription
	{
		enum class Defaults
		{
			CPUWritableVertexBuffer,
			GPUOnlyVertexBuffer,
			TransferSourceBuffer,
			CPUWritableIndexBuffer,
			UniformBuffer,
			ReadbackStorageBuffer,
		};

		VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;
		operator VmaMemoryUsage() const { return MemoryUsage; }

		VmaAllocationCreateFlags AllocationCreateFlags = 0;
		// operator VmaAllocationCreateFlags() const { return AllocationCreateFlags; }

		VkBufferUsageFlags BufferUsageFlags = 0;
		operator VkBufferUsageFlags() const { return BufferUsageFlags; }

		VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		operator VkDescriptorType() const { return DescriptorType; }

		VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE;
		operator VkSharingMode() const { return SharingMode; }

		BufferDescription() = default;
		BufferDescription(Defaults option);

		bool IsPersistentlyMapped() const;
		bool IsGpuOnly() const;
		bool IsTransferSrc() const;
		bool IsTransferDst() const;
	};

	struct DataType
	{
		enum class Defaults
		{
			None = VK_FORMAT_UNDEFINED,
			Float = VK_FORMAT_R32_SFLOAT,
			Float2 = VK_FORMAT_R32G32_SFLOAT,
			Float3 = VK_FORMAT_R32G32B32_SFLOAT,
			Float4 = VK_FORMAT_R32G32B32A32_SFLOAT,
			Mat3 = VK_FORMAT_R32G32B32_SFLOAT,
			Mat4 = VK_FORMAT_R32G32B32A32_SFLOAT,
			Int = VK_FORMAT_R32_SINT,
			Int2 = VK_FORMAT_R32G32_SINT,
			Int3 = VK_FORMAT_R32G32B32_SINT,
			Int4 = VK_FORMAT_R32G32B32A32_SINT,
			Bool = VK_FORMAT_R8_UINT,
			Depth32 = VK_FORMAT_D32_SFLOAT,
			Depth24Stencil8 = VK_FORMAT_D24_UNORM_S8_UINT,
			Depth32Stencil8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
			BGRA8 = VK_FORMAT_B8G8R8A8_UNORM,
			RGBA8 = VK_FORMAT_R8G8B8A8_SRGB,
		};

		VkFormat Format;
		bool IsMat = false;
		uint32_t MatDimensions = 0;

		operator VkFormat() const { return Format; }

		DataType() = default;
		DataType(VkFormat format)
			:Format(format) {}
		DataType(Defaults option);

		uint32_t TypeSize() const;
		bool operator==(const Defaults& other) const
		{
			return *this == DataType(other);
		}

		bool operator==(const DataType& other) const
		{
			return (Format == other.Format) && 
				(IsMat == other.IsMat) && 
				(MatDimensions == other.MatDimensions);
		}
	};

	struct ImageDescription
	{
		enum class Defaults
		{
			Depth,
			DepthStencil,
			RenderTarget,
			SampledColorAttachment,
			Texture
		};

		VkImageType Type = VK_IMAGE_TYPE_2D;
		operator VkImageType() const { return Type; }

		VkImageViewType ImageViewType = VK_IMAGE_VIEW_TYPE_2D;
		operator VkImageViewType() const { return ImageViewType; }

		VkImageUsageFlags ImageUsageFlags;
		operator VkImageUsageFlags() const { return ImageUsageFlags; }

		VkImageAspectFlags ImageAspectFlags;
		//operator VkImageAspectFlags() const { return ImageAspectFlags; }

		VkFormat Format;
		operator VkFormat() const { return Format; }

		ImageDescription() = default;
		ImageDescription(Defaults options);
	};

	struct ShaderType
	{
		enum class Defaults : VkFlags
		{
			Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
			Vertex = VK_SHADER_STAGE_VERTEX_BIT,
			Compute = VK_SHADER_STAGE_COMPUTE_BIT,
			AnyHit = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
			RayGeneration = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			Intersection = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
			Miss = VK_SHADER_STAGE_MISS_BIT_KHR,
			ClosestHit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			Mesh = VK_SHADER_STAGE_MESH_BIT_NV,
		};

		VkShaderStageFlags Stage;
		operator VkShaderStageFlags() const { return Stage; }

		ShaderType() = default;
		ShaderType(Defaults option)
			: Stage(static_cast<VkShaderStageFlags>(option)) {}
		ShaderType(shaderc_shader_kind shaderc_kind);
		ShaderType(const std::string& name);

		operator shaderc_shader_kind() const
		{
			switch (Stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:			return shaderc_glsl_vertex_shader;
				case VK_SHADER_STAGE_FRAGMENT_BIT:			return shaderc_glsl_fragment_shader;
				case VK_SHADER_STAGE_COMPUTE_BIT:			return shaderc_glsl_compute_shader;
				case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:		return shaderc_glsl_anyhit_shader;
				case VK_SHADER_STAGE_RAYGEN_BIT_KHR:		return shaderc_glsl_raygen_shader;
				case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:	return shaderc_glsl_intersection_shader;
				case VK_SHADER_STAGE_MISS_BIT_KHR:			return shaderc_glsl_miss_shader;
				case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:	return shaderc_glsl_closesthit_shader;
				case VK_SHADER_STAGE_MESH_BIT_NV:			return shaderc_glsl_mesh_shader;
			}

			return (shaderc_shader_kind)0;
		}

		operator std::string() const
		{
			switch (Stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:			return "vertex";
				case VK_SHADER_STAGE_FRAGMENT_BIT:			return "fragment";
				case VK_SHADER_STAGE_COMPUTE_BIT:			return "compute";
				case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:		return "anyhit";
				case VK_SHADER_STAGE_RAYGEN_BIT_KHR:		return "raygen";
				case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:	return "intersection";
				case VK_SHADER_STAGE_MISS_BIT_KHR:			return "miss";
				case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:	return "closesthit";
				case VK_SHADER_STAGE_MESH_BIT_NV:			return "mesh";
			}

			return "";
		}

		bool operator==(const Defaults& other) const { return Stage == static_cast<VkShaderStageFlags>(other); }
		bool operator==(const ShaderType& other) const
		{
			return Stage == other.Stage;
		}
	};
};

namespace std
{
	template<>
	struct hash<Hog::ShaderType>
	{
		std::size_t operator()(const Hog::ShaderType& k) const
		{
			return hash<VkFlags>()(k.Stage);
		}
	};
}