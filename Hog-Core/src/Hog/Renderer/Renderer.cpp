#include "hgpch.h"
#include "Renderer.h"

#include "Hog/Core/Application.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Buffer.h"
#include "Hog/Renderer/Descriptor.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Core/CVars.h"
#include "Hog/ImGui/ImGuiLayer.h"

AutoCVar_Int CVar_ImageMipLevels("renderer.enableMipMapping", "Enable mip mapping for textures", 0, CVarFlags::None);

namespace Hog
{
	struct RendererStageData
	{
		RendererStage Info;
		std::vector<VkDescriptorSet> DescriptorSets;
	};

	struct FrameData
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;
		VkFence Fence;
		VkSemaphore PresentSemaphore;
		VkSemaphore RenderSemaphore;
	};

	struct RendererData
	{
		RenderGraph Graph;
		std::vector<FrameData> FrameData;
		std::vector<RendererStageData> StageData;
		DescriptorLayoutCache DescriptorLayoutCache;
		DescriptorAllocator DescriptorAllocator;

		uint32_t FrameIndex;
		uint32_t MaxFrameCount;
	};

	static RendererData s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.MaxFrameCount = *CVarSystem::Get()->GetIntCVar("renderer.frameCount");
		s_Data.FrameData.resize(s_Data.MaxFrameCount);

		for (int i = 0; i < s_Data.FrameData.size(); ++i)
		{
			s_Data.FrameData[i].CommandPool = GraphicsContext::CreateCommandPool();
			s_Data.FrameData[i].CommandBuffer = GraphicsContext::CreateCommandBuffer(s_Data.FrameData[0].CommandPool);
			s_Data.FrameData[i].Fence = GraphicsContext::CreateFence(true);
			s_Data.FrameData[i].PresentSemaphore = GraphicsContext::CreateVkSemaphore();
			s_Data.FrameData[i].RenderSemaphore = GraphicsContext::CreateVkSemaphore();
		}

		s_Data.Graph = renderGraph;
		s_Data.DescriptorAllocator.Init(GraphicsContext::GetDevice());
		s_Data.DescriptorLayoutCache.Init(GraphicsContext::GetDevice());

		auto stages = s_Data.Graph.GetFinalStages();
		s_Data.StageData.resize(stages.size());
		s_Data.StageData[0].Info = stages[0]->StageInfo;

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
					size += resource.ConstantSize;

					buffer.resize(size);
					std::memcpy(buffer.data() + offset, resource.ConstantDataPointer, resource.ConstantSize);

					offset += (uint32_t)resource.ConstantSize;
				}
			}

			specializationInfo.mapEntryCount = (uint32_t)specializationMapEntries.size();
			specializationInfo.pMapEntries = specializationMapEntries.data();
			specializationInfo.dataSize = size;
			specializationInfo.pData = buffer.data();

			s_Data.StageData[0].Info.Shader->Generate(specializationInfo);
		}
		else
		{
			s_Data.StageData[0].Info.Shader->Generate({});
		}

		for (const auto& resource : s_Data.StageData[0].Info.Resources)
		{
			if (resource.Type == ResourceType::Storage)
			{
				VkDescriptorSet set;
				VkDescriptorBufferInfo bufferInfo = { resource.Buffer->GetHandle(), 0, VK_WHOLE_SIZE };

				DescriptorBuilder::Begin(&s_Data.DescriptorLayoutCache, &s_Data.DescriptorAllocator)
					.BindBuffer(resource.Binding, &bufferInfo, BufferTypeToVkDescriptorType(resource.Buffer->GetBufferType()), VK_SHADER_STAGE_COMPUTE_BIT)
					.Build(set);

				s_Data.StageData[0].DescriptorSets.push_back(set);
			}
		}

		if (*CVarSystem::Get()->GetIntCVar("application.enableImGui"))
		{
			auto imGuiLayer = CreateRef<ImGuiLayer>();
			Application::Get().SetImGuiLayer(imGuiLayer);
		}
	}

	void Renderer::Draw()
	{
		HG_PROFILE_FUNCTION();

		auto& currentFrame = s_Data.FrameData[s_Data.FrameIndex];

		CheckVkResult(vkWaitForFences(GraphicsContext::GetDevice(), 1, &currentFrame.Fence, VK_TRUE, UINT64_MAX));
		vkResetFences(GraphicsContext::GetDevice(), 1, &currentFrame.Fence);

		vkAcquireNextImageKHR(GraphicsContext::GetDevice(), GraphicsContext::GetSwapchain(), UINT64_MAX, currentFrame.PresentSemaphore, VK_NULL_HANDLE, &s_Data.FrameIndex);

		// begin command buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		CheckVkResult(vkBeginCommandBuffer(currentFrame.CommandBuffer, &beginInfo));

		for (auto& stage : s_Data.StageData)
		{
			for (auto& resource : stage.Info.Resources)
			{
				if (resource.Type == ResourceType::Storage)
				{
					resource.Buffer->LockAfterWrite(currentFrame.CommandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
				}
			}

			stage.Info.Shader->Bind(currentFrame.CommandBuffer);
			vkCmdBindDescriptorSets(currentFrame.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, stage.Info.Shader->GetPipelineLayout(),
				0, (uint32_t)stage.DescriptorSets.size(), stage.DescriptorSets.data(), 0, 0);

			vkCmdDispatch(currentFrame.CommandBuffer, 32, 1, 1);

			for (auto& resource : stage.Info.Resources)
			{
				if (resource.Type == ResourceType::Storage)
				{
					resource.Buffer->LockBeforeRead(currentFrame.CommandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
				}
			}
		}

		// end command buffer
		CheckVkResult(vkEndCommandBuffer(currentFrame.CommandBuffer));

		const VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = currentFrame.CommandBuffer,
		};

		const VkSemaphoreSubmitInfo waitSemaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = currentFrame.PresentSemaphore,
		};

		const VkSemaphoreSubmitInfo signalSemaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = currentFrame.RenderSemaphore,
		};

		const VkSubmitInfo2 submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitSemaphoreInfo,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &commandBufferSubmitInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalSemaphoreInfo,
		};

		CheckVkResult(vkQueueSubmit2(GraphicsContext::GetQueue(), 1, &submitInfo, currentFrame.Fence));

		s_Data.FrameIndex = (s_Data.FrameIndex + 1) % s_Data.MaxFrameCount;
	}

	void Renderer::Deinitialize()
	{
		for (const auto& frame : s_Data.FrameData)
		{
			vkDestroyFence(GraphicsContext::GetDevice(), frame.Fence, nullptr);
			vkDestroyCommandPool(GraphicsContext::GetDevice(), frame.CommandPool, nullptr);
			vkDestroySemaphore(GraphicsContext::GetDevice(), frame.PresentSemaphore, nullptr);
			vkDestroySemaphore(GraphicsContext::GetDevice(), frame.RenderSemaphore, nullptr);
		}

		s_Data.StageData.clear();
		s_Data.DescriptorLayoutCache.Cleanup();
		s_Data.DescriptorAllocator.Cleanup();
		s_Data.Graph.Cleanup();
	}

	Renderer::RendererStats Renderer::GetStats()
	{
		return RendererStats();
	}
}
