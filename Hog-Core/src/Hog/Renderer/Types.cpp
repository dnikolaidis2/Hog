#include "hgpch.h"
#include "Types.h"

namespace Hog
{
	BufferDescription::BufferDescription(Defaults option)
	{
		switch (option)
		{
		case Defaults::CPUWritableVertexBuffer:
		{
			MemoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			BufferUsageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}break;

		case Defaults::CPUWritableIndexBuffer:
		{
			MemoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			BufferUsageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}break;

		case Defaults::GPUOnlyVertexBuffer:
		{
			MemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			BufferUsageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}break;

		case Defaults::TransferSourceBuffer:
		{
			MemoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
			BufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}break;

		case Defaults::UniformBuffer:
		{
			MemoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			BufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}break;

		case Defaults::ReadbackStorageBuffer:
		{
			MemoryUsage = VMA_MEMORY_USAGE_AUTO;
			AllocationCreateFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;

			BufferUsageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}break;
		}
	}

	bool BufferDescription::IsPersistentlyMapped() const
	{
		return AllocationCreateFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	bool BufferDescription::IsGpuOnly() const
	{
		if (MemoryUsage == VMA_MEMORY_USAGE_GPU_ONLY) return true;
		return false;
	}

	bool BufferDescription::IsTransferSrc() const
	{
		if (BufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) return true;
		return false;
	}

	bool BufferDescription::IsTransferDst() const
	{
		if (BufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) return true;
		return false;
	}

	DataType::DataType(Defaults option)
	{
		Format = static_cast<VkFormat>(option);
	}

	uint32_t DataType::TypeSize() const
	{
		uint32_t result = 0;
		switch (Format)
		{
			case VK_FORMAT_UNDEFINED: result = 0; break;
			case VK_FORMAT_R4G4_UNORM_PACK8: result = 1; break;
			case VK_FORMAT_R4G4B4A4_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_R5G6B5_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_B5G6R5_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_R5G5B5A1_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_B5G5R5A1_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16: result = 2; break;
			case VK_FORMAT_R8_UNORM: result = 1; break;
			case VK_FORMAT_R8_SNORM: result = 1; break;
			case VK_FORMAT_R8_USCALED: result = 1; break;
			case VK_FORMAT_R8_SSCALED: result = 1; break;
			case VK_FORMAT_R8_UINT: result = 1; break;
			case VK_FORMAT_R8_SINT: result = 1; break;
			case VK_FORMAT_R8_SRGB: result = 1; break;
			case VK_FORMAT_R8G8_UNORM: result = 2; break;
			case VK_FORMAT_R8G8_SNORM: result = 2; break;
			case VK_FORMAT_R8G8_USCALED: result = 2; break;
			case VK_FORMAT_R8G8_SSCALED: result = 2; break;
			case VK_FORMAT_R8G8_UINT: result = 2; break;
			case VK_FORMAT_R8G8_SINT: result = 2; break;
			case VK_FORMAT_R8G8_SRGB: result = 2; break;
			case VK_FORMAT_R8G8B8_UNORM: result = 3; break;
			case VK_FORMAT_R8G8B8_SNORM: result = 3; break;
			case VK_FORMAT_R8G8B8_USCALED: result = 3; break;
			case VK_FORMAT_R8G8B8_SSCALED: result = 3; break;
			case VK_FORMAT_R8G8B8_UINT: result = 3; break;
			case VK_FORMAT_R8G8B8_SINT: result = 3; break;
			case VK_FORMAT_R8G8B8_SRGB: result = 3; break;
			case VK_FORMAT_B8G8R8_UNORM: result = 3; break;
			case VK_FORMAT_B8G8R8_SNORM: result = 3; break;
			case VK_FORMAT_B8G8R8_USCALED: result = 3; break;
			case VK_FORMAT_B8G8R8_SSCALED: result = 3; break;
			case VK_FORMAT_B8G8R8_UINT: result = 3; break;
			case VK_FORMAT_B8G8R8_SINT: result = 3; break;
			case VK_FORMAT_B8G8R8_SRGB: result = 3; break;
			case VK_FORMAT_R8G8B8A8_UNORM: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SNORM: result = 4; break;
			case VK_FORMAT_R8G8B8A8_USCALED: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SSCALED: result = 4; break;
			case VK_FORMAT_R8G8B8A8_UINT: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SINT: result = 4; break;
			case VK_FORMAT_R8G8B8A8_SRGB: result = 4; break;
			case VK_FORMAT_B8G8R8A8_UNORM: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SNORM: result = 4; break;
			case VK_FORMAT_B8G8R8A8_USCALED: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SSCALED: result = 4; break;
			case VK_FORMAT_B8G8R8A8_UINT: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SINT: result = 4; break;
			case VK_FORMAT_B8G8R8A8_SRGB: result = 4; break;
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_USCALED_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_UINT_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SINT_PACK32: result = 4; break;
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_SNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_USCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_UINT_PACK32: result = 4; break;
			case VK_FORMAT_A2R10G10B10_SINT_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_SNORM_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_USCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_UINT_PACK32: result = 4; break;
			case VK_FORMAT_A2B10G10R10_SINT_PACK32: result = 4; break;
			case VK_FORMAT_R16_UNORM: result = 2; break;
			case VK_FORMAT_R16_SNORM: result = 2; break;
			case VK_FORMAT_R16_USCALED: result = 2; break;
			case VK_FORMAT_R16_SSCALED: result = 2; break;
			case VK_FORMAT_R16_UINT: result = 2; break;
			case VK_FORMAT_R16_SINT: result = 2; break;
			case VK_FORMAT_R16_SFLOAT: result = 2; break;
			case VK_FORMAT_R16G16_UNORM: result = 4; break;
			case VK_FORMAT_R16G16_SNORM: result = 4; break;
			case VK_FORMAT_R16G16_USCALED: result = 4; break;
			case VK_FORMAT_R16G16_SSCALED: result = 4; break;
			case VK_FORMAT_R16G16_UINT: result = 4; break;
			case VK_FORMAT_R16G16_SINT: result = 4; break;
			case VK_FORMAT_R16G16_SFLOAT: result = 4; break;
			case VK_FORMAT_R16G16B16_UNORM: result = 6; break;
			case VK_FORMAT_R16G16B16_SNORM: result = 6; break;
			case VK_FORMAT_R16G16B16_USCALED: result = 6; break;
			case VK_FORMAT_R16G16B16_SSCALED: result = 6; break;
			case VK_FORMAT_R16G16B16_UINT: result = 6; break;
			case VK_FORMAT_R16G16B16_SINT: result = 6; break;
			case VK_FORMAT_R16G16B16_SFLOAT: result = 6; break;
			case VK_FORMAT_R16G16B16A16_UNORM: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SNORM: result = 8; break;
			case VK_FORMAT_R16G16B16A16_USCALED: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SSCALED: result = 8; break;
			case VK_FORMAT_R16G16B16A16_UINT: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SINT: result = 8; break;
			case VK_FORMAT_R16G16B16A16_SFLOAT: result = 8; break;
			case VK_FORMAT_R32_UINT: result = 4; break;
			case VK_FORMAT_R32_SINT: result = 4; break;
			case VK_FORMAT_R32_SFLOAT: result = 4; break;
			case VK_FORMAT_R32G32_UINT: result = 8; break;
			case VK_FORMAT_R32G32_SINT: result = 8; break;
			case VK_FORMAT_R32G32_SFLOAT: result = 8; break;
			case VK_FORMAT_R32G32B32_UINT: result = 12; break;
			case VK_FORMAT_R32G32B32_SINT: result = 12; break;
			case VK_FORMAT_R32G32B32_SFLOAT: result = 12; break;
			case VK_FORMAT_R32G32B32A32_UINT: result = 16; break;
			case VK_FORMAT_R32G32B32A32_SINT: result = 16; break;
			case VK_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
			case VK_FORMAT_R64_UINT: result = 8; break;
			case VK_FORMAT_R64_SINT: result = 8; break;
			case VK_FORMAT_R64_SFLOAT: result = 8; break;
			case VK_FORMAT_R64G64_UINT: result = 16; break;
			case VK_FORMAT_R64G64_SINT: result = 16; break;
			case VK_FORMAT_R64G64_SFLOAT: result = 16; break;
			case VK_FORMAT_R64G64B64_UINT: result = 24; break;
			case VK_FORMAT_R64G64B64_SINT: result = 24; break;
			case VK_FORMAT_R64G64B64_SFLOAT: result = 24; break;
			case VK_FORMAT_R64G64B64A64_UINT: result = 32; break;
			case VK_FORMAT_R64G64B64A64_SINT: result = 32; break;
			case VK_FORMAT_R64G64B64A64_SFLOAT: result = 32; break;
			case VK_FORMAT_B10G11R11_UFLOAT_PACK32: result = 4; break;
			case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: result = 4; break;
		default:
			break;
		}

		return result;
	}

	ImageDescription::ImageDescription(Defaults options)
	{
		switch (options)
		{
			case Defaults::Depth:
			{
				ImageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				ImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
			}break;

			case Defaults::RenderTarget:
			{
				ImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			}break;

			case Defaults::SampledColorAttachment:
			{
				ImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			}break;
			case Defaults::Texture:
			{
				ImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			}break;
		}
	}

	ShaderType::ShaderType(shaderc_shader_kind shaderc_kind)
	{
		switch (shaderc_kind)
		{
			case shaderc_glsl_vertex_shader:		Stage = VK_SHADER_STAGE_VERTEX_BIT;
			case shaderc_glsl_fragment_shader:		Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			case shaderc_glsl_compute_shader:		Stage = VK_SHADER_STAGE_COMPUTE_BIT;
			case shaderc_glsl_anyhit_shader:		Stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			case shaderc_glsl_raygen_shader:		Stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			case shaderc_glsl_intersection_shader:	Stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			case shaderc_glsl_miss_shader:			Stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			case shaderc_glsl_closesthit_shader:	Stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			case shaderc_glsl_mesh_shader:			Stage = VK_SHADER_STAGE_MESH_BIT_NV;
		}
	}

	ShaderType::ShaderType(const std::string& name)
	{
		if (name == "vertex")
			Stage = VK_SHADER_STAGE_VERTEX_BIT;
		if (name == "fragment")
			Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		if (name == "compute")
			Stage = VK_SHADER_STAGE_COMPUTE_BIT;
		if (name == "anyhit")
			Stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (name == "raygen")
			Stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if (name == "intersection")
			Stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (name == "miss")
			Stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		if (name == "closesthit")
			Stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (name == "mesh")
			Stage = VK_SHADER_STAGE_MESH_BIT_NV;
	}

	/*static VkFormat SpirvBaseTypeToVkFormat(const spirv_cross::SPIRType& type)
	{
		switch (type.basetype) {
		case spirv_cross::SPIRType::SByte:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R8_SINT;
			case 2: return VK_FORMAT_R8G8_SINT;
			case 3: return VK_FORMAT_R8G8B8_SINT;
			case 4: return VK_FORMAT_R8G8B8A8_SINT;
			}
			break;
		case spirv_cross::SPIRType::UByte:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R8_UINT;
			case 2: return VK_FORMAT_R8G8_UINT;
			case 3: return VK_FORMAT_R8G8B8_UINT;
			case 4: return VK_FORMAT_R8G8B8A8_UINT;
			}
			break;
		case spirv_cross::SPIRType::Short:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R16_SINT;
			case 2: return VK_FORMAT_R16G16_SINT;
			case 3: return VK_FORMAT_R16G16B16_SINT;
			case 4: return VK_FORMAT_R16G16B16A16_SINT;
			}
			break;
		case spirv_cross::SPIRType::UShort:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R16_UINT;
			case 2: return VK_FORMAT_R16G16_UINT;
			case 3: return VK_FORMAT_R16G16B16_UINT;
			case 4: return VK_FORMAT_R16G16B16A16_UINT;
			}
			break;
		case spirv_cross::SPIRType::Half:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R16_SFLOAT;
			case 2: return VK_FORMAT_R16G16_SFLOAT;
			case 3: return VK_FORMAT_R16G16B16_SFLOAT;
			case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
			}
			break;
		case spirv_cross::SPIRType::Int:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R32_SINT;
			case 2: return VK_FORMAT_R32G32_SINT;
			case 3: return VK_FORMAT_R32G32B32_SINT;
			case 4: return VK_FORMAT_R32G32B32A32_SINT;
			}
			break;
		case spirv_cross::SPIRType::UInt:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R32_UINT;
			case 2: return VK_FORMAT_R32G32_UINT;
			case 3: return VK_FORMAT_R32G32B32_UINT;
			case 4: return VK_FORMAT_R32G32B32A32_UINT;
			}
			break;
		case spirv_cross::SPIRType::Float:
			switch (type.vecsize) {
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			break;
		default:
			break;
		}
		return VK_FORMAT_UNDEFINED;
	}*/
};