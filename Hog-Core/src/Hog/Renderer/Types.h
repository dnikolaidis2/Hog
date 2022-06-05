#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <shaderc/shaderc.h>

namespace Hog
{
	enum class ImageLayout
	{
		Undefined									= VK_IMAGE_LAYOUT_UNDEFINED,
		General										= VK_IMAGE_LAYOUT_GENERAL,
		ColorAttachmentOptimal						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		DepthStencilAttachmentOptimal				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		DepthStencilReadOnlyOptimal					= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		ShaderReadOnlyOptimal						= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		TransferSrcOptimal							= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		TransferDstOptimal							= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		Preinitialized								= VK_IMAGE_LAYOUT_PREINITIALIZED,
		DepthReadOnlyStencilAttachmentOptimal		= VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
		DepthAttachmentStencilReadOnlyOptimal		= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
		DepthAttachmentOptimal						= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		DepthReadOnlyOptimal						= VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
		StencilAttachmentOptimal					= VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
		StencilReadOnlyOptimal						= VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
		ReadOnlyOptimal								= VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
		AttachmentOptimal							= VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		PresentSrcKHR								= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		SharedPresentKHR							= VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
		FragmentDensityMapOptimalExt				= VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
		FragmentShadingRateAttachmentOptimalKHR		= VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
		DepthReadOnlyStencilAttachmentOptimalKHR	= VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR,
		DepthAttachmentStencilReadOnlyOptimalKHR	= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR,
		ShadingRateOptimalNV						= VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV,
		DepthAttachmentOptimalKHR					= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR,
		DepthReadOnlyOptimalKHR						= VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR,
		StencilAttachmentOptimalKHR					= VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR,
		StencilReadOnlyOptimalKHR					= VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR,
		ReadOnlyOptimalKHR							= VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
		AttachmentOptimalKHR						= VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
		MaxEnum										= VK_IMAGE_LAYOUT_MAX_ENUM,
	};

	/*
	vec3f Position;
	vec3f Normal;
	vec2f TexCoords;
	int32 MaterialIndex;
	*/


	struct Vertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoords;
		glm::vec3 Normal;
		glm::vec4 Tangent;
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
			VertexBuffer,
			IndexBuffer,
			UniformBuffer,
			ReadbackStorageBuffer,
			AccelerationStructureBuildInput,
			AccelerationStructure,
			AccelerationStructureScratchBuffer,
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
			None			= VK_FORMAT_UNDEFINED,
			Float			= VK_FORMAT_R32_SFLOAT,
			Float2			= VK_FORMAT_R32G32_SFLOAT,
			Float3			= VK_FORMAT_R32G32B32_SFLOAT,
			Float4			= VK_FORMAT_R32G32B32A32_SFLOAT,
			Mat3			= VK_FORMAT_R32G32B32_SFLOAT,
			Mat4			= VK_FORMAT_R32G32B32A32_SFLOAT,
			Int				= VK_FORMAT_R32_SINT,
			Int2			= VK_FORMAT_R32G32_SINT,
			Int3			= VK_FORMAT_R32G32B32_SINT,
			Int4			= VK_FORMAT_R32G32B32A32_SINT,
			Bool			= VK_FORMAT_R8_UINT,
			Depth32			= VK_FORMAT_D32_SFLOAT,
			Depth24Stencil8 = VK_FORMAT_D24_UNORM_S8_UINT,
			Depth32Stencil8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
			BGRA8			= VK_FORMAT_B8G8R8A8_UNORM,
			RGBA8			= VK_FORMAT_R8G8B8A8_SRGB,
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

		VkFormat Format = VK_FORMAT_UNDEFINED;
		operator VkFormat() const { return Format; }

		VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		operator VkImageLayout() const { return ImageLayout; }

		ImageDescription() = default;
		ImageDescription(Defaults options);
	};

	struct ShaderType
	{
		enum class Defaults : VkFlags
		{
			Fragment			= VK_SHADER_STAGE_FRAGMENT_BIT,
			Vertex				= VK_SHADER_STAGE_VERTEX_BIT,
			Compute				= VK_SHADER_STAGE_COMPUTE_BIT,
			AnyHit				= VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
			RayGeneration		= VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			Intersection		= VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
			Miss				= VK_SHADER_STAGE_MISS_BIT_KHR,
			ClosestHit			= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			Mesh				= VK_SHADER_STAGE_MESH_BIT_NV,
			CombinedFragVert	= VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
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

	enum class AttachmentType
	{
		Color, Depth, DepthStencil, Swapchain
	};

	enum class ResourceType
	{
		Uniform, Constant, PushConstant, Storage, Sampler, SamplerArray
	};

	enum class RendererStageType
	{
		ForwardCompute, DeferredCompute, ForwardGraphics, DeferredGraphics, Blit, ImGui, Barrier
	};

	static inline VkPipelineBindPoint ToPipelineBindPoint(RendererStageType type)
	{
		switch (type)
		{
			case RendererStageType::ForwardCompute:		return VK_PIPELINE_BIND_POINT_COMPUTE;
			case RendererStageType::DeferredCompute:	return VK_PIPELINE_BIND_POINT_COMPUTE;
			case RendererStageType::ForwardGraphics:	return VK_PIPELINE_BIND_POINT_GRAPHICS;
			case RendererStageType::DeferredGraphics:	return VK_PIPELINE_BIND_POINT_GRAPHICS;
			case RendererStageType::Blit:				return VK_PIPELINE_BIND_POINT_GRAPHICS;
			case RendererStageType::ImGui:				return VK_PIPELINE_BIND_POINT_GRAPHICS;
		}

		return (VkPipelineBindPoint)0;
	}

	enum class PipelineStage : VkPipelineStageFlags2
	{
		None								= VK_PIPELINE_STAGE_2_NONE,
		TopOfPipe							= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		DrawIndirect						= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
		VertexInput							= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
		VertexShader						= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		TessellationControlShader			= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
		TessellationEvaluationShader		= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
		GeometryShader						= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
		FragmentShader						= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		EarlyFragmentTests					= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		LateFragmentTests					= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		ColorAttachmentOutput				= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		ComputeShader						= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		AllTransfer							= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
		Transfer							= VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		BottomOfPipe						= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		Host								= VK_PIPELINE_STAGE_2_HOST_BIT,
		AllGraphics							= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
		AllCommands							= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		Copy								= VK_PIPELINE_STAGE_2_COPY_BIT,
		Resolve								= VK_PIPELINE_STAGE_2_RESOLVE_BIT,
		Blit								= VK_PIPELINE_STAGE_2_BLIT_BIT,
		Clear								= VK_PIPELINE_STAGE_2_CLEAR_BIT,
		IndexInput							= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
		VertexAttributeInput				= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
		PreRasterizationShaders				= VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
		TransformFeedbackExt				= VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT,
		ConditionalRenderingExt				= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT,
		CommandPreprocessNV					= VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV,
		FragmentShadingRateAttachmentKHR	= VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR,
		ShadingRateImageNV					= VK_PIPELINE_STAGE_2_SHADING_RATE_IMAGE_BIT_NV,
		AccelerationStructureBuildKHR		= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		RayTracingShaderKHR					= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		FragmentDensityProcessExt			= VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
		TaskShaderNV						= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV,
		MeshShaderNV						= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV,
	};

	static inline VkPipelineStageFlags ToStageFlags1(VkPipelineStageFlags2 flag)
	{
		switch (flag)
		{
			case VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT: 							return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			case VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT: 						return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			case VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT: 							return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			case VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT: 						return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			case VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT: 			return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
			case VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT: 		return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			case VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT: 						return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			case VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT: 						return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			case VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT: 					return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			case VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT: 					return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			case VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT: 				return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			case VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT: 						return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			case VK_PIPELINE_STAGE_2_TRANSFER_BIT: 								return VK_PIPELINE_STAGE_TRANSFER_BIT;
			case VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT: 						return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			case VK_PIPELINE_STAGE_2_HOST_BIT: 									return VK_PIPELINE_STAGE_HOST_BIT;
			case VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT: 							return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			case VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT: 							return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			case VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT: 				return VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
			case VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT: 			return VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
			case VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV: 				return VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV;
			case VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR: 	return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
			case VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR: 		return VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
			case VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR: 				return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			case VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT: 			return VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
			case VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV: 						return VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
			case VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV: 						return VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
		}

		return VK_PIPELINE_STAGE_NONE;
	}

	enum class AccessFlag : VkAccessFlags2
	{
		None									= VK_ACCESS_2_NONE,
		IndirectCommandRead						= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
		IndexRead								= VK_ACCESS_2_INDEX_READ_BIT,
		VertexAttributeRead						= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
		UniformRead								= VK_ACCESS_2_UNIFORM_READ_BIT,
		InputAttachmentRead						= VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT,
		ShaderRead								= VK_ACCESS_2_SHADER_READ_BIT,
		ShaderWrite								= VK_ACCESS_2_SHADER_WRITE_BIT,
		ColorAttachmentRead						= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
		ColorAttachmentWrite					= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		DepthStencilAttachmentRead				= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		DepthStencilAttachmentWrite				= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		TransferRead							= VK_ACCESS_2_TRANSFER_READ_BIT,
		TransferWrite							= VK_ACCESS_2_TRANSFER_WRITE_BIT,
		HostRead								= VK_ACCESS_2_HOST_READ_BIT,
		HostWrite								= VK_ACCESS_2_HOST_WRITE_BIT,
		MemoryRead								= VK_ACCESS_2_MEMORY_READ_BIT,
		MemoryWrite								= VK_ACCESS_2_MEMORY_WRITE_BIT,
		ShaderSampledRead						= VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		ShaderStorageRead						= VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
		ShaderStorageWrite						= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		TransformFeedbackWriteExt				= VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,
		TransformFeedbackCounterReadExt			= VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT,
		TransformFeedbackCounterWriteExt		= VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT,
		ConditionalRenderingReadExt				= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT,
		CommandPreprocessReadNV					= VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV,
		CommandPreprocessWriteNV				= VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV,
		FragmentShadingRateAttachmentReadKHR	= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
		ShadingRateImageReadNV					= VK_ACCESS_2_SHADING_RATE_IMAGE_READ_BIT_NV,
		AccelerationStructureReadKHR			= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		AccelerationStructureWriteKHR			= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		FragmentDensityMapReadExt				= VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT,
		ColorAttachmentReadNoncoherentExt		= VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
	};

	static inline VkAccessFlags ToAccessFlags1(VkAccessFlags2 flag)
	{
		switch (flag)
		{
			case VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT: 						return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			case VK_ACCESS_2_INDEX_READ_BIT: 									return VK_ACCESS_INDEX_READ_BIT;
			case VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT: 						return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			case VK_ACCESS_2_UNIFORM_READ_BIT: 									return VK_ACCESS_UNIFORM_READ_BIT;
			case VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT: 						return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			case VK_ACCESS_2_SHADER_READ_BIT: 									return VK_ACCESS_SHADER_READ_BIT;
			case VK_ACCESS_2_SHADER_WRITE_BIT: 									return VK_ACCESS_SHADER_WRITE_BIT;
			case VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT: 						return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			case VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT: 						return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			case VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT: 				return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			case VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT: 				return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			case VK_ACCESS_2_TRANSFER_READ_BIT: 								return VK_ACCESS_TRANSFER_READ_BIT;
			case VK_ACCESS_2_TRANSFER_WRITE_BIT: 								return VK_ACCESS_TRANSFER_WRITE_BIT;
			case VK_ACCESS_2_HOST_READ_BIT: 									return VK_ACCESS_HOST_READ_BIT;
			case VK_ACCESS_2_HOST_WRITE_BIT: 									return VK_ACCESS_HOST_WRITE_BIT;
			case VK_ACCESS_2_MEMORY_READ_BIT: 									return VK_ACCESS_MEMORY_READ_BIT;
			case VK_ACCESS_2_MEMORY_WRITE_BIT: 									return VK_ACCESS_MEMORY_WRITE_BIT;
			case VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT: 					return VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
			case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT: 			return VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT;
			case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT: 			return VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
			case VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT: 				return VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
			case VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV: 					return VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV;
			case VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV: 					return VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV;
			case VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR: 	return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
			case VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR: 				return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			case VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR: 				return VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			case VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT: 				return VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
			case VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT: 		return VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
		}

		return VK_ACCESS_2_NONE;
	}

	struct BarrierDescription
	{
		enum class Defaults
		{
			WaitColorAttachmentBeforeDraw,
		};

		PipelineStage SrcStage = PipelineStage::None;
		AccessFlag SrcAccessMask = AccessFlag::None;
		PipelineStage DstStage = PipelineStage::None;
		AccessFlag DstAccessMask = AccessFlag::None;
		ImageLayout OldLayout = ImageLayout::Undefined;
		ImageLayout NewLayout = ImageLayout::Undefined;

		BarrierDescription() = default;
		BarrierDescription(PipelineStage srcStage, AccessFlag srcAccessMask, PipelineStage dstStage, AccessFlag dstAccessMask, ImageLayout oldLayout, ImageLayout newLayout)
			: SrcStage(srcStage), SrcAccessMask(srcAccessMask), DstStage(dstStage), DstAccessMask(dstAccessMask), OldLayout(oldLayout), NewLayout(newLayout) {}
		BarrierDescription(ImageLayout oldLayout, ImageLayout newLayout)
			: OldLayout(oldLayout), NewLayout(newLayout) {}
		BarrierDescription(PipelineStage srcStage, AccessFlag srcAccessMask, PipelineStage dstStage, AccessFlag dstAccessMask)
			: SrcStage(srcStage), SrcAccessMask(srcAccessMask), DstStage(dstStage), DstAccessMask(dstAccessMask) {}
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