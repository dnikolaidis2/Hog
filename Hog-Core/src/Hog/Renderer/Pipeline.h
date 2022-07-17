#pragma once

#include <volk.h>

#include "Hog/Renderer/Types.h"
#include "Hog/Renderer/Shader.h"

namespace Hog
{
	class Pipeline
	{
	public:
		~Pipeline();

		virtual void Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo) = 0;
		virtual void Bind(VkCommandBuffer commandBuffer) = 0;

		VkPipeline GetHandle() { return m_Handle; }
		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
	protected:
		void AddShader(std::string shader);
		void AddShaderStage(ShaderType type, VkShaderModule shaderModule, VkSpecializationInfo* specializationInfo, const char* main = "main");
	protected:
		std::unordered_map<ShaderType, Ref<ShaderSource>> m_ShaderSources;
		std::unordered_map<ShaderType, VkShaderModule> m_ShaderModules;
		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageCreateInfos;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;;
		VkPipeline m_Handle = VK_NULL_HANDLE;
	};

	class GraphicsPipeline : public Pipeline
	{
	public:
		struct Configuration
		{
			std::vector<std::string> Shaders;

			// VkPipelineInputAssemblyStateCreateInfo
			struct InputAssemblyState
			{
				PrimitiveType Topology = PrimitiveType::TriangleList;
			} Input;
			
			// VkPipelineRasterizationStateCreateInfo
			struct RasterizationState
			{
				bool DiscardEnable 				= false;
				PolygonMode PolygonMode 		= PolygonMode::Fill;
				CullMode CullMode 				= CullMode::Back;
				FrontFace FrontFace 			= FrontFace::CounterClockwise;
				float LineWidth 				= 1.0f;
				struct DepthOptions
				{
					bool ClampEnable 			= false;
					bool BiasEnable 			= false;
					float BiasConstantFactor 	= 0.0f;
					float BiasClamp 			= 0.0f;
					float BiasSlopeFactor 		= 0.0f;
				} Depth;
			} Rasterizer;

			// VkPipelineColorBlendAttachmentState
			struct ColorBlendAttachment
			{
				bool Enable 					= true;

				BlendFactor SrcColorFactor		= BlendFactor::SrcAlpha;
				BlendFactor DstColorFactor		= BlendFactor::OneMinusSrcAlpha;
				BlendOp ColorOp 				= BlendOp::Add;
				BlendFactor SrcAlphaFactor		= BlendFactor::One;
				BlendFactor DstAlphaFactor		= BlendFactor::Zero;
				BlendOp AlphaOp 				= BlendOp::Add;
				ColorComponent ColorWriteMask	= ColorComponent::RGBA;
			};

			std::vector<ColorBlendAttachment> BlendAttachments {{}};

			// VkPipelineColorBlendStateCreateInfo
			struct ColorBlend
			{
				bool LogicOpEnable		= false;
				LogicOp LogicOp			= LogicOp::Copy;
				glm::vec4 BlendConstants {0.0f, 0.0f, 0.0f, 0.0f};
			} Blend;
			
			// VkPipelineMultisampleStateCreateInfo
			struct Multisample
			{
				bool SampleShadingEnable	= false;
				float MinSampleShading		= .2f; // Optional
			}Multisample;

			// VkPipelineDepthStencilStateCreateInfo
			struct DepthStencil
			{
				bool DepthTestEnable = true;
				bool DepthWriteEnable = true;
				CompareOp DepthCompareOp = CompareOp::LessOrEqual;
				bool DepthBoundsTestEnable = false;
				bool StencilTestEnable = false;
				struct StencilOpSate
				{
					StencilOp FailOp {};
					StencilOp PassOp {};
					StencilOp DepthFailOp {};
					CompareOp CompareOp {};
					uint32_t CompareMask {};
					uint32_t WriteMask {};
					uint32_t Reference {};
				};
				
				StencilOpSate Front {};
				StencilOpSate Back {};
				float MinBounds = 0.0f; // Optional
				float MaxBounds = 0.0f; // Optional
			}DepthStencil;

			std::vector<DynamicState> DynamicStates { DynamicState::Viewport, DynamicState::Scissor };
		};
	public:
		static Ref<Pipeline> Create(const Configuration& configuration);
	public:
		GraphicsPipeline(const Configuration& configuration);
		~GraphicsPipeline();

		virtual void Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo) override;
		virtual void Bind(VkCommandBuffer commandBuffer) override;
	private:
		Configuration m_Config;

		VkPipelineVertexInputStateCreateInfo m_VertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		};

		VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStateCreateInfo 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		};

		VkViewport m_Viewport 
		{
			.x = 0.0f,
			.y = 0.0f,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D m_Scissor 
		{
			.offset = {0, 0}
		};

		VkPipelineViewportStateCreateInfo m_ViewportStateCreateInfo 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &m_Viewport,
			.scissorCount = 1,
			.pScissors = &m_Scissor,
		};

		VkPipelineRasterizationStateCreateInfo m_RasterizationStateCreateInfo 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		};

		std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStates;

		VkPipelineColorBlendStateCreateInfo m_ColorBlendStateCreateInfo 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		};

		VkPipelineMultisampleStateCreateInfo m_MultisamplingStateCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
		};

		VkPipelineDepthStencilStateCreateInfo m_PipelineDepthStencilCreateInfo 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		};

		std::vector<VkDynamicState> m_DynamicStates;

		VkPipelineDynamicStateCreateInfo m_DynamicStateCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		};

		VkGraphicsPipelineCreateInfo m_GraphicsPipelineCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pVertexInputState = &m_VertexInputStateCreateInfo,
			.pInputAssemblyState = &m_InputAssemblyStateCreateInfo,
			.pViewportState = &m_ViewportStateCreateInfo,
			.pRasterizationState = &m_RasterizationStateCreateInfo,
			.pMultisampleState = &m_MultisamplingStateCreateInfo,
			.pDepthStencilState = &m_PipelineDepthStencilCreateInfo, // Optional
			.pColorBlendState = &m_ColorBlendStateCreateInfo,
			.pDynamicState = &m_DynamicStateCreateInfo,
		};
	};

	class ComputePipeline : public Pipeline
	{
	public:
		struct Configuration
		{
			std::string Shader;
		};
	public:
		static Ref<Pipeline> Create(const Configuration& configuration);
	public:
		ComputePipeline(const Configuration& configuration);
		~ComputePipeline();

		virtual void Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo) override;
		virtual void Bind(VkCommandBuffer commandBuffer) override;
	private:
		Configuration m_Config;

		VkComputePipelineCreateInfo m_ComputePipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		};
	};

	class RayTracingPipeline : public Pipeline
	{
	public:
		struct Configuration
		{
			std::vector<std::string> Shaders;
			uint32_t MaxRayRecursionDepth = 2;
		};
	public:
		static Ref<Pipeline> Create(const Configuration& configuration);
	public:
		RayTracingPipeline(const Configuration& configuration);

		virtual void Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo) override;
		virtual void Bind(VkCommandBuffer commandBuffer) override;
	private:
		Configuration m_Config;

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups;

		VkRayTracingPipelineCreateInfoKHR m_RayTracingPipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
		};
	};
}
