#include "hgpch.h"
#include "Renderer.h"

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Core/CVars.h"

AutoCVar_Int CVar_ImageMipLevels("renderer.enableMipMapping", "Enable mip mapping for textures", 0, CVarFlags::None);

namespace Hog
{
	struct RendererStageData
	{
		RendererStage Info;
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;
		VkFence Fence;
	};

	struct RendererData
	{
		RenderGraph Graph;
		std::vector<RendererStageData> StageData;
	};

	static RendererData s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.Graph = renderGraph;
		auto stages = s_Data.Graph.GetFinalStages();
		s_Data.StageData.resize(stages.size());
		s_Data.StageData[0].Info = stages[0]->StageInfo;
		s_Data.StageData[0].CommandPool = GraphicsContext::CreateCommandPool();
		s_Data.StageData[0].CommandBuffer = GraphicsContext::CreateCommandBuffer(s_Data.StageData[0].CommandPool);
		s_Data.StageData[0].Fence = GraphicsContext::CreateFence(false);

		if (s_Data.StageData[0].Info.Resources.ContainsType(ResourceType::Constant))
		{
			VkSpecializationInfo specializationInfo;
			std::vector<VkSpecializationMapEntry> specializationMapEntries;
			std::vector<uint8_t> buffer;

			uint32_t offset = 0;
			size_t size = 0;

			for (const auto& resource : s_Data.StageData[0].Info.Resources)
			{
				if (resource.Type == ResourceType::Constant)
				{
					specializationMapEntries.push_back({ resource.ConstantID, offset, resource.ConstantSize });

					offset += resource.ConstantSize;
					size += resource.ConstantSize;

					buffer.resize(size);
					std::memcpy(buffer.data() + offset, resource.ConstantDataPointer, resource.ConstantSize);
				}
			}

			specializationInfo.mapEntryCount = specializationMapEntries.size();
			specializationInfo.pMapEntries = specializationMapEntries.data();
			specializationInfo.dataSize = size;
			specializationInfo.pData = buffer.data();
			
			s_Data.StageData[0].Info.Shader->Generate(specializationInfo);
		}
		else
		{
			s_Data.StageData[0].Info.Shader->Generate({});
		}
	}

	void Renderer::Begin()
	{
		HG_PROFILE_FUNCTION();
	}

	void Renderer::End()
	{
		HG_PROFILE_FUNCTION();
	}

	void Renderer::Draw()
	{
		HG_PROFILE_FUNCTION();

		for (auto& stage : s_Data.StageData)
		{
			// begin command buffer
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			CheckVkResult(vkBeginCommandBuffer(stage.CommandBuffer, &beginInfo));

			for (auto& resource : stage.Info.Resources)
			{
				if (resource.Type == ResourceType::Uniform)
				{
					resource.Buffer->LockAfterWrite(stage.CommandBuffer, VK_SHADER_STAGE_COMPUTE_BIT);
				}
			}

			stage.Info.Shader->Bind(stage.CommandBuffer);
			//bind descriptor;

			vkCmdDispatch(stage.CommandBuffer, 32, 1, 1);

			for (auto& resource : stage.Info.Resources)
			{
				if (resource.Type == ResourceType::Uniform)
				{
					resource.Buffer->LockBeforeRead(stage.CommandBuffer, VK_SHADER_STAGE_COMPUTE_BIT);
				}
			}

			// Submit compute work
			vkResetFences(GraphicsContext::GetDevice(), 1, &stage.Fence);

			// end command buffer
			CheckVkResult(vkEndCommandBuffer(stage.CommandBuffer));

			const VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = stage.CommandBuffer,
			};

			const VkSubmitInfo2 submitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.commandBufferInfoCount = 1,
				.pCommandBufferInfos = &commandBufferSubmitInfo,
			};

			CheckVkResult(vkQueueSubmit2(GraphicsContext::GetComputeQueue(), 1, &submitInfo, stage.Fence));
			CheckVkResult(vkWaitForFences(GraphicsContext::GetDevice(), 1, &stage.Fence, VK_TRUE, UINT64_MAX));
		}
	}

	void Renderer::Deinitialize()
	{
	}

	Renderer::RendererStats Renderer::GetStats()
	{
		return RendererStats();
	}
}
