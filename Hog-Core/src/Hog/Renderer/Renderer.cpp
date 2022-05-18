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
	struct BlitStageData
	{
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		Ref<Shader> Shader;

		void Init()
		{
			Shader = Hog::Shader::Create("Blit", "fullscreen.vertex", "blit.fragment");
			CreateRenderPass();
		}

		void Cleanup()
		{
			Shader.reset();
			vkDestroyRenderPass(GraphicsContext::GetDevice(), RenderPass, nullptr);
		}

		void CreateRenderPass()
		{
			//we define an attachment description for our main color image
			//the attachment is loaded as "clear" when renderpass start
			//the attachment is stored when renderpass ends
			//the attachment layout starts as "undefined", and transitions to "Present" so its possible to display it
			//we dont care about stencil, and dont use multisampling

			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = GraphicsContext::GetSwapchainFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			//we are going to create 1 subpass, which is the minimum you can do
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			//1 dependency, which is from "outside" into the subpass. And we can read or write color
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo render_pass_info = {};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			//2 attachments from said array
			render_pass_info.attachmentCount = 1;
			render_pass_info.pAttachments = &colorAttachment;
			render_pass_info.subpassCount = 1;
			render_pass_info.pSubpasses = &subpass;
			//render_pass_info.dependencyCount = 1;
			//render_pass_info.pDependencies = &dependency;

			CheckVkResult(vkCreateRenderPass(GraphicsContext::GetDevice(), &render_pass_info, nullptr, &RenderPass));
		}
	};

	struct ImGuiStageData
	{
		VkRenderPass RenderPass;
	};

	struct RendererStageData
	{
		RendererStage Info;
		VkRenderPass RenderPass;
	};

	struct FrameData
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;
		VkFence Fence;
		VkSemaphore PresentSemaphore;
		VkSemaphore RenderSemaphore;
		VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
		DescriptorAllocator DescriptorAllocator;

		void Init()
		{
			CommandPool = GraphicsContext::CreateCommandPool();
			CommandBuffer = GraphicsContext::CreateCommandBuffer(CommandPool);
			Fence = GraphicsContext::CreateFence(true);
			PresentSemaphore = GraphicsContext::CreateVkSemaphore();
			RenderSemaphore = GraphicsContext::CreateVkSemaphore();
			DescriptorAllocator.Init(GraphicsContext::GetDevice());
		}

		void Cleanup()
		{
			vkDestroyFence(GraphicsContext::GetDevice(), Fence, nullptr);
			vkDestroyCommandPool(GraphicsContext::GetDevice(), CommandPool, nullptr);
			vkDestroySemaphore(GraphicsContext::GetDevice(), PresentSemaphore, nullptr);
			vkDestroySemaphore(GraphicsContext::GetDevice(), RenderSemaphore, nullptr);
			vkDestroyFramebuffer(GraphicsContext::GetDevice(), FrameBuffer, nullptr);
			DescriptorAllocator.Cleanup();
		}
	};

	struct RendererData
	{
		RenderGraph Graph;
		std::vector<FrameData> FrameData;
		std::vector<RendererStageData> StageData;
		ImGuiStageData ImGuiStage;
		BlitStageData BlitStage;
		DescriptorLayoutCache DescriptorLayoutCache;

		uint32_t FrameIndex = 0;
		uint32_t MaxFrameCount = 2;
	};

	static RendererData s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.MaxFrameCount = *CVarSystem::Get()->GetIntCVar("renderer.frameCount");
		s_Data.FrameData.resize(s_Data.MaxFrameCount);
		std::for_each(s_Data.FrameData.begin(), s_Data.FrameData.end(), [](FrameData& elem) {elem.Init(); });

		s_Data.BlitStage.Init();

		s_Data.Graph = renderGraph;
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

			std::vector<VkDescriptorSet> sets;
			for (const auto& resource : stage.Info.Resources)
			{
				if (resource.Type == ResourceType::Storage)
				{
					VkDescriptorSet set;
					VkDescriptorBufferInfo bufferInfo = { resource.Buffer->GetHandle(), 0, VK_WHOLE_SIZE };

					DescriptorBuilder::Begin(&s_Data.DescriptorLayoutCache, &currentFrame.DescriptorAllocator)
						.BindBuffer(resource.Binding, &bufferInfo, BufferTypeToVkDescriptorType(resource.Buffer->GetBufferType()), VK_SHADER_STAGE_COMPUTE_BIT)
						.Build(set);

					sets.push_back(set);
				}
			}

			vkCmdBindDescriptorSets(currentFrame.CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, stage.Info.Shader->GetPipelineLayout(),
				0, (uint32_t)sets.size(), sets.data(), 0, 0);

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
		std::for_each(s_Data.FrameData.begin(), s_Data.FrameData.end(), [](FrameData& elem) {elem.Cleanup(); });
		s_Data.FrameData.clear();
		s_Data.BlitStage.Cleanup();
		s_Data.StageData.clear();
		s_Data.DescriptorLayoutCache.Cleanup();
		s_Data.Graph.Cleanup();
	}

	Renderer::RendererStats Renderer::GetStats()
	{
		return RendererStats();
	}
}
