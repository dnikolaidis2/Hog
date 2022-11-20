#include <hgpch.h>

#include "Pipeline.h"

#include <Hog/Utils/RendererUtils.h>
#include <Hog/Renderer/GraphicsContext.h>
#include <Hog/Renderer/Resource/Shader.h>

namespace Hog
{
	Pipeline::~Pipeline()
	{
		for (auto& [type, module] : m_ShaderModules)
		{
			vkDestroyShaderModule(GraphicsContext::GetDevice(), module, nullptr);
		}

		vkDestroyPipelineLayout(GraphicsContext::GetDevice(), m_PipelineLayout, nullptr);
		vkDestroyPipeline(GraphicsContext::GetDevice(), m_Handle, nullptr);
	}

	void Pipeline::AddShader(std::string shader)
	{
		auto shadeSource = ShaderCache::GetShader(shader);
		if (m_ShaderSources.find(shadeSource->Type) != m_ShaderSources.end())
		{
			HG_CORE_WARN("Replacing existing source in shader!");
		}

		m_ShaderSources[shadeSource->Type] = shadeSource;
	}

	void Pipeline::AddShaderStage(ShaderType type, VkShaderModule shaderModule, VkSpecializationInfo* specializationInfo, const char* main)
	{
		VkPipelineShaderStageCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = static_cast<VkShaderStageFlagBits>(type.Stage),
			.module = shaderModule,
			.pName = main,
			.pSpecializationInfo = specializationInfo,
		};

		m_ShaderStageCreateInfos.push_back(info);
	}

	Ref<Pipeline> GraphicsPipeline::Create(const Configuration& configuration)
	{
		return CreateRef<GraphicsPipeline>(configuration);
	}

	GraphicsPipeline::GraphicsPipeline(const Configuration& configuration)
		: m_Config(configuration)
	{
		std::for_each(m_Config.Shaders.begin(), m_Config.Shaders.end(), 
			[this](const std::string& shader) {
				AddShader(shader);
			});

		m_InputAssemblyStateCreateInfo.topology = static_cast<VkPrimitiveTopology>(m_Config.Input.Topology);

		m_RasterizationStateCreateInfo.depthClampEnable = m_Config.Rasterizer.Depth.ClampEnable;
		m_RasterizationStateCreateInfo.rasterizerDiscardEnable = m_Config.Rasterizer.DiscardEnable;
		m_RasterizationStateCreateInfo.polygonMode = static_cast<VkPolygonMode>(m_Config.Rasterizer.PolygonMode);
		m_RasterizationStateCreateInfo.cullMode = static_cast<VkCullModeFlags>(m_Config.Rasterizer.CullMode);
		m_RasterizationStateCreateInfo.frontFace = static_cast<VkFrontFace>(m_Config.Rasterizer.FrontFace);
		m_RasterizationStateCreateInfo.depthBiasEnable = m_Config.Rasterizer.Depth.BiasEnable;
		m_RasterizationStateCreateInfo.depthBiasConstantFactor = m_Config.Rasterizer.Depth.BiasConstantFactor;
		m_RasterizationStateCreateInfo.depthBiasClamp = m_Config.Rasterizer.Depth.BiasClamp;
		m_RasterizationStateCreateInfo.depthBiasSlopeFactor = m_Config.Rasterizer.Depth.BiasSlopeFactor;
		m_RasterizationStateCreateInfo.lineWidth = m_Config.Rasterizer.LineWidth;

		std::for_each(m_Config.BlendAttachments.begin(), m_Config.BlendAttachments.end(),
			[this](Configuration::ColorBlendAttachment& blendAttachment)
			{
				VkPipelineColorBlendAttachmentState result{};
				result.blendEnable = blendAttachment.Enable;
				result.srcColorBlendFactor = static_cast<VkBlendFactor>(blendAttachment.SrcColorFactor);
				result.dstColorBlendFactor = static_cast<VkBlendFactor>(blendAttachment.DstColorFactor);
				result.colorBlendOp = static_cast<VkBlendOp>(blendAttachment.ColorOp);
				result.srcAlphaBlendFactor = static_cast<VkBlendFactor>(blendAttachment.SrcAlphaFactor);
				result.dstAlphaBlendFactor = static_cast<VkBlendFactor>(blendAttachment.DstAlphaFactor);
				result.alphaBlendOp = static_cast<VkBlendOp>(blendAttachment.AlphaOp);
				result.colorWriteMask = static_cast<VkColorComponentFlags>(blendAttachment.ColorWriteMask);
				m_ColorBlendAttachmentStates.emplace_back(result);
			});

		m_ColorBlendStateCreateInfo.logicOpEnable = m_Config.Blend.LogicOpEnable;
		m_ColorBlendStateCreateInfo.logicOp = static_cast<VkLogicOp>(m_Config.Blend.LogicOp);
		m_ColorBlendStateCreateInfo.blendConstants[0] = m_Config.Blend.BlendConstants.r;
		m_ColorBlendStateCreateInfo.blendConstants[1] = m_Config.Blend.BlendConstants.g;
		m_ColorBlendStateCreateInfo.blendConstants[2] = m_Config.Blend.BlendConstants.b;
		m_ColorBlendStateCreateInfo.blendConstants[3] = m_Config.Blend.BlendConstants.a;
		m_ColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(m_ColorBlendAttachmentStates.size());
		m_ColorBlendStateCreateInfo.pAttachments = m_ColorBlendAttachmentStates.data();

		m_MultisamplingStateCreateInfo.sampleShadingEnable = m_Config.Multisample.SampleShadingEnable;
		m_MultisamplingStateCreateInfo.minSampleShading = m_Config.Multisample.MinSampleShading;
		m_MultisamplingStateCreateInfo.rasterizationSamples = GraphicsContext::GetMSAASamples();

		m_PipelineDepthStencilCreateInfo.depthTestEnable = m_Config.DepthStencil.DepthTestEnable;
		m_PipelineDepthStencilCreateInfo.depthWriteEnable = m_Config.DepthStencil.DepthWriteEnable;
		m_PipelineDepthStencilCreateInfo.depthCompareOp = static_cast<VkCompareOp>(m_Config.DepthStencil.DepthCompareOp);
		m_PipelineDepthStencilCreateInfo.depthBoundsTestEnable = m_Config.DepthStencil.DepthBoundsTestEnable;
		m_PipelineDepthStencilCreateInfo.stencilTestEnable = m_Config.DepthStencil.StencilTestEnable;
		m_PipelineDepthStencilCreateInfo.front.failOp = static_cast<VkStencilOp>(m_Config.DepthStencil.Front.FailOp);
		m_PipelineDepthStencilCreateInfo.front.passOp = static_cast<VkStencilOp>(m_Config.DepthStencil.Front.PassOp);
		m_PipelineDepthStencilCreateInfo.front.depthFailOp = static_cast<VkStencilOp>(m_Config.DepthStencil.Front.DepthFailOp);
		m_PipelineDepthStencilCreateInfo.front.compareOp = static_cast<VkCompareOp>(m_Config.DepthStencil.Front.CompareOp);
		m_PipelineDepthStencilCreateInfo.front.compareMask = m_Config.DepthStencil.Front.CompareMask;
		m_PipelineDepthStencilCreateInfo.front.writeMask = m_Config.DepthStencil.Front.WriteMask;
		m_PipelineDepthStencilCreateInfo.front.reference = m_Config.DepthStencil.Front.Reference;
		m_PipelineDepthStencilCreateInfo.back.failOp = static_cast<VkStencilOp>(m_Config.DepthStencil.Back.FailOp);
		m_PipelineDepthStencilCreateInfo.back.passOp = static_cast<VkStencilOp>(m_Config.DepthStencil.Back.PassOp);
		m_PipelineDepthStencilCreateInfo.back.depthFailOp = static_cast<VkStencilOp>(m_Config.DepthStencil.Back.DepthFailOp);
		m_PipelineDepthStencilCreateInfo.back.compareOp = static_cast<VkCompareOp>(m_Config.DepthStencil.Back.CompareOp);
		m_PipelineDepthStencilCreateInfo.back.compareMask = m_Config.DepthStencil.Back.CompareMask;
		m_PipelineDepthStencilCreateInfo.back.writeMask = m_Config.DepthStencil.Back.WriteMask;
		m_PipelineDepthStencilCreateInfo.back.reference = m_Config.DepthStencil.Back.Reference;
		m_PipelineDepthStencilCreateInfo.minDepthBounds = m_Config.DepthStencil.MinBounds;
		m_PipelineDepthStencilCreateInfo.maxDepthBounds = m_Config.DepthStencil.MaxBounds;

		
		std::for_each(m_Config.DynamicStates.begin(), m_Config.DynamicStates.end(),
			[this](const DynamicState& state) 
			{
				return m_DynamicStates.emplace_back(static_cast<VkDynamicState>(state));
			});

		m_DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
		m_DynamicStateCreateInfo.pDynamicStates = m_DynamicStates.data();
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
	}

	void GraphicsPipeline::Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo)
	{
		auto data = ShaderReflection::ReflectPipelineLayout(m_ShaderSources);

		for (const auto& [stage, source] : m_ShaderSources)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = (uint32_t)source->Code.size() * sizeof(uint32_t);
			createInfo.pCode = source->Code.data();

			VkShaderModule shaderModule;
			CheckVkResult(vkCreateShaderModule(GraphicsContext::GetDevice(), &createInfo, nullptr, &shaderModule));

			AddShaderStage(stage, shaderModule, specializationInfo);
			m_ShaderModules[stage] = shaderModule;
		}


		m_PipelineLayout = data.PipelineLayout;

		m_VertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)data.VertexInputBindingDescriptions.size();
		m_VertexInputStateCreateInfo.pVertexBindingDescriptions = data.VertexInputBindingDescriptions.data(); // Optional
		m_VertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)data.VertexInputAttributeDescriptions.size();
		m_VertexInputStateCreateInfo.pVertexAttributeDescriptions = data.VertexInputAttributeDescriptions.data(); // Optional

		m_GraphicsPipelineCreateInfo.stageCount = (uint32_t)m_ShaderStageCreateInfos.size();
		m_GraphicsPipelineCreateInfo.pStages = m_ShaderStageCreateInfos.data();
		m_GraphicsPipelineCreateInfo.layout = m_PipelineLayout;
		m_GraphicsPipelineCreateInfo.renderPass = renderPass;

		CheckVkResult(vkCreateGraphicsPipelines(GraphicsContext::GetDevice(), VK_NULL_HANDLE, 1, &m_GraphicsPipelineCreateInfo, nullptr, &m_Handle));
	}

	void GraphicsPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Handle);
	}

	Ref<Pipeline> ComputePipeline::Create(const Configuration& configuration)
	{
		return CreateRef<ComputePipeline>(configuration);
	}

	ComputePipeline::ComputePipeline(const Configuration& configuration)
		: m_Config(configuration)
	{
		AddShader(m_Config.Shader);
	}

	ComputePipeline::~ComputePipeline()
	{
	}

	void ComputePipeline::Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo)
	{
		auto data = ShaderReflection::ReflectPipelineLayout(m_ShaderSources);

		m_PipelineLayout = data.PipelineLayout;
		m_ComputePipelineCreateInfo.layout = m_PipelineLayout;

		for (const auto& [stage, source] : m_ShaderSources)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = (uint32_t)source->Code.size() * sizeof(uint32_t);
			createInfo.pCode = source->Code.data();

			VkShaderModule shaderModule;
			CheckVkResult(vkCreateShaderModule(GraphicsContext::GetDevice(), &createInfo, nullptr, &shaderModule));
		
			AddShaderStage(stage, shaderModule, specializationInfo);
			m_ShaderModules[stage] = shaderModule;
		}
		
		m_ComputePipelineCreateInfo.stage = m_ShaderStageCreateInfos[0];

		CheckVkResult(vkCreateComputePipelines(GraphicsContext::GetDevice(), VK_NULL_HANDLE, 1, &m_ComputePipelineCreateInfo, nullptr, &m_Handle));
	}

	void ComputePipeline::Bind(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Handle);
	}

	Ref<Pipeline> RayTracingPipeline::Create(const Configuration& configuration)
	{
		return CreateRef<RayTracingPipeline>(configuration);
	}

	RayTracingPipeline::RayTracingPipeline(const Configuration& configuration)
		: m_Config(configuration)
	{
		std::for_each(m_Config.Shaders.begin(), m_Config.Shaders.end(),
			[this](const std::string& shader) {
				AddShader(shader);
			});
	}

	void RayTracingPipeline::Generate(VkRenderPass renderPass, VkSpecializationInfo* specializationInfo)
	{
		auto data = ShaderReflection::ReflectPipelineLayout(m_ShaderSources);

		m_PipelineLayout = data.PipelineLayout;

		for (const auto& [stage, source] : m_ShaderSources)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = (uint32_t)source->Code.size() * sizeof(uint32_t);
			createInfo.pCode = source->Code.data();

			VkShaderModule shaderModule;
			CheckVkResult(vkCreateShaderModule(GraphicsContext::GetDevice(), &createInfo, nullptr, &shaderModule));

			AddShaderStage(stage, shaderModule, specializationInfo);
			m_ShaderModules[stage] = shaderModule;

			
			if (stage == ShaderType::Defaults::ClosestHit)
			{
				VkRayTracingShaderGroupCreateInfoKHR shaderGroup{
					.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
					.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
					.generalShader = VK_SHADER_UNUSED_KHR,
					.closestHitShader = static_cast<uint32_t>(m_ShaderStageCreateInfos.size()) - 1,
					.anyHitShader = VK_SHADER_UNUSED_KHR,
					.intersectionShader = VK_SHADER_UNUSED_KHR,
				};

				m_ShaderGroups.push_back(shaderGroup);
			}
			else
			{
				VkRayTracingShaderGroupCreateInfoKHR shaderGroup{
					.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
					.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
					.generalShader = static_cast<uint32_t>(m_ShaderStageCreateInfos.size()) - 1,
					.closestHitShader = VK_SHADER_UNUSED_KHR,
					.anyHitShader = VK_SHADER_UNUSED_KHR,
					.intersectionShader = VK_SHADER_UNUSED_KHR,
				};

				m_ShaderGroups.push_back(shaderGroup);
			}
		}

		m_RayTracingPipelineCreateInfo.stageCount = static_cast<uint32_t>(m_ShaderStageCreateInfos.size());
		m_RayTracingPipelineCreateInfo.pStages = m_ShaderStageCreateInfos.data();
		m_RayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
		m_RayTracingPipelineCreateInfo.pGroups = m_ShaderGroups.data();
		
		m_RayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = m_Config.MaxRayRecursionDepth;
		
		m_RayTracingPipelineCreateInfo.layout = m_PipelineLayout;

		CheckVkResult(vkCreateRayTracingPipelinesKHR(GraphicsContext::GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &m_RayTracingPipelineCreateInfo, nullptr, &m_Handle));
	}

	void RayTracingPipeline::Bind(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_FUNCTION();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Handle);
	}
}
