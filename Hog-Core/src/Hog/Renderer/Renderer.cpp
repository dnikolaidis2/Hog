#include "hgpch.h"
#include "Renderer.h"

#include "Hog/Renderer/FrameBuffer.h"
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
	struct RendererStage;
	struct RendererFrame;

	struct RendererData
	{
		RenderGraph Graph;
		std::vector<RendererFrame> Frames;
		std::vector<RendererStage> Stages;
		DescriptorLayoutCache DescriptorLayoutCache;
		Ref<Image> FinalTarget;
		Ref<ImGuiLayer> ImGuiLayer;

		uint32_t FrameIndex = 0;
		uint32_t MaxFrameCount = 2;

		RendererFrame& GetCurrentFrame()
		{
			return Frames[FrameIndex];
		}
	};

	static RendererData s_Data;

	struct RendererFrame
	{
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		VkFence Fence = VK_NULL_HANDLE;
		VkSemaphore PresentSemaphore = VK_NULL_HANDLE;
		VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
		FrameBuffer FrameBuffer;
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

		void Init(Ref<Image> swapchainImage, VkRenderPass renderPass)
		{
			CommandPool = GraphicsContext::CreateCommandPool();
			CommandBuffer = GraphicsContext::CreateCommandBuffer(CommandPool);
			Fence = GraphicsContext::CreateFence(true);
			PresentSemaphore = GraphicsContext::CreateVkSemaphore();
			RenderSemaphore = GraphicsContext::CreateVkSemaphore();
			DescriptorAllocator.Init(GraphicsContext::GetDevice());

			std::vector<Ref<Image>> attachments(1);
			attachments[0] = swapchainImage;
			FrameBuffer.Create(attachments, renderPass);

		}

		void BeginFrame()
		{
			VkExtent2D swapchainExtent = GraphicsContext::GetExtent();
			VkClearValue clearValue = {
				.color = { { 0.1f, 0.1f, 0.1f, 1.0f } },
			};

			CheckVkResult(vkWaitForFences(GraphicsContext::GetDevice(), 1, &Fence, VK_TRUE, UINT64_MAX));
			vkResetFences(GraphicsContext::GetDevice(), 1, &Fence);

			DescriptorAllocator.ResetPools();

			vkAcquireNextImageKHR(GraphicsContext::GetDevice(), GraphicsContext::GetSwapchain(), UINT64_MAX, PresentSemaphore, VK_NULL_HANDLE, &s_Data.FrameIndex);

			// begin command buffer
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			CheckVkResult(vkBeginCommandBuffer(CommandBuffer, &beginInfo));
			HG_PROFILE_GPU_CONTEXT(currentFrame.CommandBuffer);
			HG_PROFILE_GPU_EVENT("Begin CommandBuffer");

			/*s_Data.FinalTarget->LayoutBarrier(CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			s_Data.FinalTarget->SetImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/
		}

		void EndFrame()
		{
			// end command buffer
			CheckVkResult(vkEndCommandBuffer(CommandBuffer));

			// Submit
			const VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = CommandBuffer,
			};

			const VkSemaphoreSubmitInfo waitSemaphoreInfo = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = PresentSemaphore,
			};

			const VkSemaphoreSubmitInfo signalSemaphoreInfo = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = RenderSemaphore,
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

			CheckVkResult(vkQueueSubmit2(GraphicsContext::GetQueue(), 1, &submitInfo, Fence));

			// Present
			if (s_Data.FinalTarget)
			{
				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = &RenderSemaphore;

				VkSwapchainKHR swapChains[] = { GraphicsContext::GetSwapchain() };
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapChains;

				presentInfo.pImageIndices = &s_Data.FrameIndex;

				HG_PROFILE_GPU_FLIP(GraphicsContext::GetSwapchain());

				CheckVkResult(vkQueuePresentKHR(GraphicsContext::GetQueue(), &presentInfo));
			}
		}

		void Cleanup()
		{
			vkDestroyFence(GraphicsContext::GetDevice(), Fence, nullptr);
			vkDestroyCommandPool(GraphicsContext::GetDevice(), CommandPool, nullptr);
			vkDestroySemaphore(GraphicsContext::GetDevice(), PresentSemaphore, nullptr);
			vkDestroySemaphore(GraphicsContext::GetDevice(), RenderSemaphore, nullptr);
			DescriptorAllocator.Cleanup();
			FrameBuffer.Cleanup();
		}
	};

	struct RendererStage
	{
		StageDescription Info;
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		FrameBuffer FrameBuffer;

		void Init()
		{
			if (Info.StageType == RendererStageType::DeferredGraphics || Info.StageType == RendererStageType::ForwardGraphics
				|| Info.StageType == RendererStageType::ImGui || Info.StageType == RendererStageType::Blit)
			{
				std::vector<VkAttachmentDescription2> attachments(Info.Attachments.size());

				for (int i = 0; i < attachments.size(); ++i)
				{
					attachments[i] = {};
					attachments[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
					attachments[i].format = Info.Attachments[i].Image->GetDescription().Format;
					attachments[i].samples = Info.Attachments[i].Image->GetSamples();
					attachments[i].loadOp = (Info.Attachments[i].Clear) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
					attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					if (Info.Attachments[i].Type == AttachmentType::DepthStencil)
					{
						attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
					}
					attachments[i].initialLayout = Info.Attachments[i].Image->GetDescription().ImageLayout;

					attachments[i].finalLayout = Info.Attachments[i].Image->GetDescription().ImageLayout;

					if (Info.Attachments[i].Present)
					{
						attachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						Info.Attachments[i].NextImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
					}

					if (Info.Attachments[i].UseAsResourceNext)
					{
						attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						Info.Attachments[i].NextImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}
				}

				std::vector<VkAttachmentReference2> attachmentRefs(Info.Attachments.size());

				for (int i = 0; i < attachmentRefs.size(); ++i)
				{
					attachmentRefs[i] = {};
					attachmentRefs[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
					attachmentRefs[i].attachment = i;
					attachmentRefs[i].layout = Info.Attachments[i].Image->GetDescription().ImageLayout;
				}

				//we are going to create 1 subpass, which is the minimum you can do
				VkSubpassDescription2 subpass = {
					.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size()),
					.pColorAttachments = attachmentRefs.data(),
				};

				//1 dependency, which is from "outside" into the subpass. And we can read or write color
				VkSubpassDependency2 dependency = {
					.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				};

				VkRenderPassCreateInfo2 renderPassInfo = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
					.attachmentCount = static_cast<uint32_t>(attachments.size()),
					.pAttachments = attachments.data(),
					.subpassCount = 1,
					.pSubpasses = &subpass,
					//.dependencyCount = 1,
					//.pDependencies = &dependency,
				};

				CheckVkResult(vkCreateRenderPass2(GraphicsContext::GetDevice(), &renderPassInfo, nullptr, &RenderPass));
			}

			VkSpecializationInfo specializationInfo = {};
			std::vector<VkSpecializationMapEntry> specializationMapEntries;
			std::vector<uint8_t> buffer;

			if (Info.Shader)
			{
				if (Info.Resources.ContainsType(ResourceType::Constant))
				{
					uint32_t offset = 0;
					size_t size = 0;

					for (const auto& resource : Info.Resources)
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
				}

				if (RenderPass != VK_NULL_HANDLE)
				{
					Info.Shader->Generate(RenderPass, specializationInfo);
				}
				else
				{
					Info.Shader->Generate(specializationInfo);
				}
			}

			if (Info.StageType != RendererStageType::Blit && RenderPass != VK_NULL_HANDLE)
			{
				auto attachments = Info.Attachments.GetElements();
				std::vector<Ref<Image>> fbAttachments(attachments.size());
				for (int i = 0; i < attachments.size(); ++i)
				{
					fbAttachments[i] = attachments[i].Image;
				}

				FrameBuffer.Create(fbAttachments, RenderPass, fbAttachments[0]->GetExtent());
			}
		}

		void Execute(VkCommandBuffer commandBuffer)
		{
			switch(Info.StageType)
			{
				case RendererStageType::Blit:
				{
					BlitStage(commandBuffer);
				}break;
				case RendererStageType::DeferredCompute:
				case RendererStageType::ForwardCompute:
				{
					ForwardCompute(commandBuffer);
				}break;
				case RendererStageType::ImGui:
				{
					ImGui(commandBuffer);
				}break;
				case RendererStageType::ForwardGraphics:
				case RendererStageType::DeferredGraphics:
					break;
			}

			for (auto & attachment: Info.Attachments)
			{
				attachment.Image->SetImageLayout(attachment.NextImageLayout);
			}
		}

		void Cleanup()
		{
			FrameBuffer.Cleanup();
			vkDestroyRenderPass(GraphicsContext::GetDevice(), RenderPass, nullptr);
		}
	private:
		void ForwardCompute(VkCommandBuffer commandBuffer)
		{
			HG_PROFILE_GPU_EVENT("ForwardCompute Pass");

			Info.Shader->Bind(commandBuffer);

			std::vector<VkDescriptorSet> sets;
			for (const auto& resource : Info.Resources)
			{
				if (resource.Type == ResourceType::Storage)
				{
					VkDescriptorSet set;
					VkDescriptorBufferInfo bufferInfo = { resource.Buffer->GetHandle(), 0, VK_WHOLE_SIZE };

					DescriptorBuilder::Begin(&s_Data.DescriptorLayoutCache, &s_Data.GetCurrentFrame().DescriptorAllocator)
						.BindBuffer(resource.Binding, &bufferInfo, resource.Buffer->GetBufferDescription(), resource.BindLocation)
						.Build(set);

					sets.push_back(set);
				}
			}

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Info.Shader->GetPipelineLayout(),
				0, (uint32_t)sets.size(), sets.data(), 0, 0);

			vkCmdDispatch(commandBuffer, Info.GroupCounts.x, Info.GroupCounts.y, Info.GroupCounts.z);
		}

		void ImGui(VkCommandBuffer commandBuffer)
		{
			// ImGui
			HG_PROFILE_GPU_EVENT("ImGui Pass");

			VkRenderPassBeginInfo renderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = RenderPass,
				.framebuffer = static_cast<VkFramebuffer>(FrameBuffer),
				.renderArea = {
					.extent = FrameBuffer.GetExtent(),
				},
			};

			VkSubpassBeginInfo subpassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.contents = VK_SUBPASS_CONTENTS_INLINE,
			};

			vkCmdBeginRenderPass2(commandBuffer, &renderPassBeginInfo, &subpassBeginInfo);

			// Imgui draw
			s_Data.ImGuiLayer->Draw(commandBuffer);

			vkCmdEndRenderPass(commandBuffer);
		}

		void BlitStage(VkCommandBuffer commandBuffer)
		{
			// Copy to final target
			HG_PROFILE_GPU_EVENT("Blit Pass");
			RendererFrame& currentFrame = s_Data.GetCurrentFrame();
			VkExtent2D extent = currentFrame.FrameBuffer.GetExtent();

			VkRenderPassBeginInfo renderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = RenderPass,
				.framebuffer = static_cast<VkFramebuffer>(currentFrame.FrameBuffer),
				.renderArea = {
					.extent = extent
				}
			};

			VkSubpassBeginInfo subpassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.contents = VK_SUBPASS_CONTENTS_INLINE,
			};

			vkCmdBeginRenderPass2(commandBuffer, &renderPassBeginInfo, &subpassBeginInfo);

			VkViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(extent.width);
			viewport.height = static_cast<float>(extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = extent;

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			vkCmdSetDepthBias(commandBuffer, 0, 0, 0);

			Info.Shader->Bind(commandBuffer);

			VkDescriptorImageInfo sourceImage;
			sourceImage.sampler = Info.Resources[0].Texture->GetOrCreateSampler();

			sourceImage.imageView = Info.Resources[0].Texture->GetImageView();
			sourceImage.imageLayout = Info.Resources[0].Texture->GetDescription().ImageLayout;

			VkDescriptorSet blitSet;
			DescriptorBuilder::Begin(&s_Data.DescriptorLayoutCache, &currentFrame.DescriptorAllocator)
				.BindImage(0, &sourceImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Info.Resources[0].BindLocation)
				.Build(blitSet);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Info.Shader->GetPipelineLayout(),
				0, 1, &blitSet, 0, nullptr);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffer);
		}
	};

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.MaxFrameCount = *CVarSystem::Get()->GetIntCVar("renderer.frameCount");

		s_Data.Graph = renderGraph;
		// auto finalStages = renderGraph.GetFinalStages();

		/*// Add ImGui to RenderGraph and add it to the end
		Ref<Node> imGuiStage;
		if (*CVarSystem::Get()->GetIntCVar("application.enableImGui") && !s_Data.Graph.ContainsStageType(RendererStageType::ImGui))
		{
			imGuiStage = s_Data.Graph.AddStage(finalStages, {
				"ImGuiStage", RendererStageType::ImGui, {{"ColorTarget", AttachmentType::Color, GetFinalRenderTarget()}}
			});
		}

		// Add Blit stage to RenderGraph and add it to the end
		Ref<Node> blitStage;
		if (!s_Data.Graph.ContainsStageType(RendererStageType::Blit))
		{
			StageDescription blitStageDesc = {
				"BlitStage", Shader::Create("Blit", "fullscreen.vertex", "blit.fragment"), RendererStageType::Blit,
				{{"FinalRender", ResourceType::Sampler, ShaderType::Defaults::Fragment, GetFinalRenderTarget(), 0, 0},},
				{{"ColorTarget", AttachmentType::Color, GetFinalRenderTarget(), false, true},},
			};

			if (imGuiStage)
			{
				blitStage = s_Data.Graph.AddStage(imGuiStage, blitStageDesc);
			}
			else
			{
				blitStage = s_Data.Graph.AddStage(finalStages, blitStageDesc);
			}
		}*/

		/*for (auto& parent : blitStage->ParentList)
		{
			for (auto& attachment : parent->StageInfo.Attachments)
			{
				if (attachment.Image == GetFinalRenderTarget())
				{
					attachment.UseAsResourceNext = true;
				}
			}
		}*/

		s_Data.DescriptorLayoutCache.Init(GraphicsContext::GetDevice());

		auto stages = s_Data.Graph.GetStages();
		s_Data.Stages.resize(stages.size());

		VkRenderPass blitRenderPass = VK_NULL_HANDLE;

		for (int i = 0; i < s_Data.Stages.size(); ++i)
		{
			auto& stage = s_Data.Stages[i];
			stage.Info = stages[i]->StageInfo;

			stage.Init();

			if (stage.Info.StageType == RendererStageType::ImGui)
			{
				s_Data.ImGuiLayer = CreateRef<ImGuiLayer>(stage.RenderPass);
				Application::Get().SetImGuiLayer(s_Data.ImGuiLayer);
			}

			if (stage.Info.StageType == RendererStageType::Blit)
			{
				blitRenderPass = stage.RenderPass;
			}
		}

		s_Data.Frames.resize(s_Data.MaxFrameCount);
		if (s_Data.FinalTarget)
		{
			for (int i = 0; i < s_Data.Frames.size(); ++i)
			{
				s_Data.Frames[i].Init(GraphicsContext::GetSwapchainImages()[i], blitRenderPass);
			}
		}
		else
		{
			for (int i = 0; i < s_Data.Frames.size(); ++i)
			{
				s_Data.Frames[i].Init();
			}
		}
	}

	void Renderer::Draw()
	{
		HG_PROFILE_FUNCTION();

		auto& currentFrame = s_Data.Frames[s_Data.FrameIndex];

		currentFrame.BeginFrame();

		for (auto& stage : s_Data.Stages)
		{
			stage.Execute(currentFrame.CommandBuffer);
		}

		currentFrame.EndFrame();

		s_Data.FrameIndex = (s_Data.FrameIndex + 1) % s_Data.MaxFrameCount;
	}

	void Renderer::Deinitialize()
	{
		s_Data.FinalTarget.reset();
		std::for_each(s_Data.Frames.begin(), s_Data.Frames.end(), [](RendererFrame& elem) {elem.Cleanup(); });
		s_Data.Frames.clear();
		std::for_each(s_Data.Stages.begin(), s_Data.Stages.end(), [](RendererStage& elem) {elem.Cleanup(); });
		s_Data.Stages.clear();
		s_Data.DescriptorLayoutCache.Cleanup();
		s_Data.Graph.Cleanup();
		
		Application::Get().PopOverlay(s_Data.ImGuiLayer);
		s_Data.ImGuiLayer.reset();
	}

	void Renderer::SetFinalRenderTarget(Ref<Image> image)
	{
		s_Data.FinalTarget = image;
	}

	Ref<Image> Renderer::GetFinalRenderTarget()
	{
		if (!s_Data.FinalTarget)
		{
			VkExtent2D extent = GraphicsContext::GetExtent();
			s_Data.FinalTarget = Image::Create(ImageDescription::Defaults::SampledColorAttachment, 1);
		}

		return s_Data.FinalTarget;
	}

	Renderer::RendererStats Renderer::GetStats()
	{
		return RendererStats();
	}
}
