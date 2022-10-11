#include "hgpch.h"
#include "Renderer.h"

#include "Hog/Core/Application.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Buffer.h"
#include "Hog/Utils/RendererUtils.h"
#include "Hog/Core/CVars.h"
#include "Hog/ImGui/ImGuiLayer.h"

AutoCVar_Int CVar_ImageMipLevels("renderer.enableMipMapping", "Enable mip mapping for textures", 0, CVarFlags::None);

namespace Hog
{
	struct RendererData
	{
		RenderGraph Graph;
		std::vector<RendererFrame> Frames;
		std::vector<RendererStage> Stages;
		bool Present = false;
		DescriptorLayoutCache DescriptorLayoutCache;
		Ref<ImGuiLayer> ImGuiLayer;

		uint32_t FrameIndex = 0;
		uint32_t MaxFrameCount = 2;

		RendererFrame& GetCurrentFrame()
		{
			return Frames[FrameIndex];
		}
	};

	static RendererData s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.MaxFrameCount = *CVarSystem::Get()->GetIntCVar("renderer.frameCount");

		s_Data.Graph = renderGraph;

		s_Data.DescriptorLayoutCache.Init(GraphicsContext::GetDevice());

		auto stages = s_Data.Graph.GetStages();
		s_Data.Stages.resize(stages.size());

		VkRenderPass blitRenderPass = VK_NULL_HANDLE;

		for (int i = 0; i < s_Data.Stages.size(); ++i)
		{
			auto& stage = s_Data.Stages[i];
			stage.Info = stages[i]->StageInfo;

			stage.Init();

			switch (stage.Info.StageType)
			{
				case RendererStageType::ImGui:
				{
					s_Data.ImGuiLayer = CreateRef<ImGuiLayer>(stage.RenderPass);
					Application::Get().SetImGuiLayer(s_Data.ImGuiLayer);
					s_Data.Present = true;
				}break;

				case RendererStageType::Blit:
				{
					blitRenderPass = stage.RenderPass;
					s_Data.Present = true;
				}break;

				case RendererStageType::ForwardGraphics:
				case RendererStageType::DeferredGraphics:
				case RendererStageType::ScreenSpacePass:
				{
					s_Data.Present = true;
				}break;

				default:
				{
					s_Data.Present = false;
				}break;
			}
		}

		s_Data.Frames.resize(s_Data.MaxFrameCount);
		if (s_Data.Present)
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

	void Renderer::Cleanup()
	{
		std::for_each(s_Data.Frames.begin(), s_Data.Frames.end(), [](RendererFrame& elem) {elem.Cleanup(); });
		s_Data.Frames.clear();
		std::for_each(s_Data.Stages.begin(), s_Data.Stages.end(), [](RendererStage& elem) {elem.Cleanup(); });
		s_Data.Stages.clear();
		s_Data.DescriptorLayoutCache.Cleanup();
		s_Data.Graph.Cleanup();
		
		Application::Get().PopOverlay(s_Data.ImGuiLayer);
		s_Data.ImGuiLayer.reset();
	}

	DescriptorLayoutCache* Renderer::GetDescriptorLayoutCache()
	{
		return &(s_Data.DescriptorLayoutCache);
	}

	Renderer::RendererStats Renderer::GetStats()
	{
		return RendererStats();
	}

	void RendererFrame::Init()
	{
		Device = GraphicsContext::GetDevice();
		Queue = GraphicsContext::GetQueue();
		Swapchain = GraphicsContext::GetSwapchain();
		CommandPool = GraphicsContext::CreateCommandPool();
		CommandBuffer = GraphicsContext::CreateCommandBuffer(CommandPool);
		Fence = GraphicsContext::CreateFence(true);
		PresentSemaphore = GraphicsContext::CreateVkSemaphore();
		RenderSemaphore = GraphicsContext::CreateVkSemaphore();
		DescriptorAllocator.Init(Device);
	}

	void RendererFrame::Init(Ref<Image> swapchainImage, VkRenderPass renderPass)
	{
		Device = GraphicsContext::GetDevice();
		Queue = GraphicsContext::GetQueue();
		Swapchain = GraphicsContext::GetSwapchain();
		CommandPool = GraphicsContext::CreateCommandPool();
		CommandBuffer = GraphicsContext::CreateCommandBuffer(CommandPool);
		Fence = GraphicsContext::CreateFence(true);
		PresentSemaphore = GraphicsContext::CreateVkSemaphore();
		RenderSemaphore = GraphicsContext::CreateVkSemaphore();
		DescriptorAllocator.Init(Device);
		SwapchainImage = swapchainImage;
		std::vector<Ref<Image>> attachments(1);
		attachments[0] = SwapchainImage;
		FrameBuffer = FrameBuffer::Create(attachments, renderPass);
	}

	void RendererFrame::BeginFrame()
	{
		CheckVkResult(vkWaitForFences(Device, 1, &Fence, VK_TRUE, UINT64_MAX));
		vkResetFences(Device, 1, &Fence);

		DescriptorAllocator.ResetPools();

		vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, PresentSemaphore, VK_NULL_HANDLE, &s_Data.FrameIndex);

		// begin command buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		CheckVkResult(vkBeginCommandBuffer(CommandBuffer, &beginInfo));
		HG_PROFILE_GPU_CONTEXT(currentFrame.CommandBuffer);
		HG_PROFILE_GPU_EVENT("Begin CommandBuffer");

		if (SwapchainImage)
		{
			SwapchainImage->ExecuteBarrier(CommandBuffer, {ImageLayout::Undefined, ImageLayout::ColorAttachmentOptimal});
		}
	}

	void RendererFrame::EndFrame()
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

		CheckVkResult(vkQueueSubmit2(Queue, 1, &submitInfo, Fence));

		// Present
		if (s_Data.Present)
		{
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &RenderSemaphore;

			VkSwapchainKHR swapChains[] = { Swapchain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &s_Data.FrameIndex;

			HG_PROFILE_GPU_FLIP(Swapchain);

			CheckVkResult(vkQueuePresentKHR(Queue, &presentInfo));
		}
	}

	void RendererFrame::Cleanup()
	{
		vkDestroyFence(Device, Fence, nullptr);
		vkDestroyCommandPool(Device, CommandPool, nullptr);
		vkDestroySemaphore(Device, PresentSemaphore, nullptr);
		vkDestroySemaphore(Device, RenderSemaphore, nullptr);
		DescriptorAllocator.Cleanup();
		FrameBuffer.reset();
	}

	void RendererStage::Init()
	{
		if (Info.StageType == RendererStageType::DeferredGraphics || Info.StageType == RendererStageType::ForwardGraphics
			|| Info.StageType == RendererStageType::ImGui || Info.StageType == RendererStageType::Blit
			|| Info.StageType == RendererStageType::ScreenSpacePass)
		{
			std::vector<VkAttachmentDescription2> attachments(Info.Attachments.size());
			std::unordered_map<AttachmentType, std::vector<VkAttachmentReference2>> attachmentRefs;
			std::vector<VkSubpassDependency2> dependencies;
			dependencies.reserve(Info.Attachments.size());
			ClearValues.resize(Info.Attachments.size());

			for (int i = 0; i < attachments.size(); ++i)
			{
				attachments[i] = {};
				attachments[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
				if (Info.Attachments[i].Type != AttachmentType::Swapchain)
				{
					attachments[i].format = Info.Attachments[i].Image->GetFormat();
					attachments[i].samples = Info.Attachments[i].Image->GetSamples();
				}
				else
				{
					attachments[i].format = GraphicsContext::GetSwapchainFormat();
					attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
				}
				attachments[i].loadOp = (Info.Attachments[i].Clear) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

				if (Info.Attachments[i].Barrier.OldLayout == ImageLayout::Undefined)
				{
					attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				}

				attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				if (Info.Attachments[i].Type == AttachmentType::DepthStencil)
				{
					attachments[i].stencilLoadOp = (Info.Attachments[i].Clear) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
					attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
				}
				attachments[i].initialLayout = static_cast<VkImageLayout>(Info.Attachments[i].Barrier.OldLayout);
				attachments[i].finalLayout = static_cast<VkImageLayout>(Info.Attachments[i].Barrier.NewLayout);

				VkAttachmentReference2 attachRef = {
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.attachment = static_cast<uint32_t>(i),
				};

				switch (Info.Attachments[i].Type)
				{
					case AttachmentType::Color:
					case AttachmentType::Swapchain:		attachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
					case AttachmentType::Depth:			attachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
					case AttachmentType::DepthStencil:	attachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
				}

				if (Info.Attachments[i].Type == AttachmentType::Swapchain)
				{
					attachmentRefs[AttachmentType::Color].push_back(attachRef);
				}
				else
				{
					attachmentRefs[Info.Attachments[i].Type].push_back(attachRef);
				}

				VkSubpassDependency2 dependency = {
					.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.srcStageMask = ToStageFlags1(static_cast<VkPipelineStageFlags2>(Info.Attachments[i].Barrier.SrcStage)),
					.dstStageMask = ToStageFlags1(static_cast<VkPipelineStageFlags2>(Info.Attachments[i].Barrier.DstStage)),
					.srcAccessMask = ToAccessFlags1(static_cast<VkAccessFlags2>(Info.Attachments[i].Barrier.SrcAccessMask)),
					.dstAccessMask = ToAccessFlags1(static_cast<VkAccessFlags2>(Info.Attachments[i].Barrier.DstAccessMask)),
				};

				dependencies.push_back(dependency);

				if (Info.Attachments[i].Clear)
				{
					if (Info.Attachments[i].Type == AttachmentType::Color ||
						Info.Attachments[i].Type == AttachmentType::Swapchain)
					{
						ClearValues[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
					}
					else if (Info.Attachments[i].Type == AttachmentType::Depth ||
						Info.Attachments[i].Type == AttachmentType::DepthStencil)
					{
						ClearValues[i].depthStencil.depth = 1.f;
					}
				}
			}

			//we are going to create 1 subpass, which is the minimum you can do
			VkSubpassDescription2 subpass = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			};

			if (attachmentRefs.contains(AttachmentType::Color))
			{
				subpass.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs[AttachmentType::Color].size());
				subpass.pColorAttachments = attachmentRefs[AttachmentType::Color].data();
			}

			if (attachmentRefs.contains(AttachmentType::Depth))
			{
				subpass.pDepthStencilAttachment= attachmentRefs[AttachmentType::Depth].data();
			}

			VkRenderPassCreateInfo2 renderPassInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.subpassCount = 1,
				.pSubpasses = &subpass,
				.dependencyCount = static_cast<uint32_t>(dependencies.size()),
				.pDependencies = dependencies.data(),
			};

			CheckVkResult(vkCreateRenderPass2(GraphicsContext::GetDevice(), &renderPassInfo, nullptr, &RenderPass));
		}

		VkSpecializationInfo specializationInfo = {};
		std::vector<VkSpecializationMapEntry> specializationMapEntries;
		std::vector<uint8_t> buffer;

		if (Info.Pipeline)
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
				uint32_t colorCount = 0;
				std::for_each(Info.Attachments.begin(), Info.Attachments.end(), [&colorCount](AttachmentElement& attachment) 
					{ 
						colorCount += (attachment.Type == AttachmentType::Color || attachment.Type == AttachmentType::Swapchain) ? 1 : 0;
					});

				Info.Pipeline->Generate(RenderPass, &specializationInfo);
			}
			else
			{
				Info.Pipeline->Generate(nullptr, &specializationInfo);
			}
		}

		if (Info.StageType == RendererStageType::RayTracing)
		{
			Info.ShaderBindingTable = ShaderBindingTable::Create(Info.Pipeline->GetHandle());
		}

		if (Info.StageType != RendererStageType::Blit && RenderPass != VK_NULL_HANDLE)
		{
			auto attachments = Info.Attachments.GetElements();
			std::vector<Ref<Image>> fbAttachments(attachments.size());
			for (int i = 0; i < attachments.size(); ++i)
			{
				fbAttachments[i] = attachments[i].Image;
			}

			FrameBuffer = FrameBuffer::Create(fbAttachments, RenderPass, fbAttachments[0]->GetExtent());
		}
	}

	void RendererStage::Execute(VkCommandBuffer commandBuffer)
	{
		/*for (const auto& resource : stage.Info.Resources)
		{
			if (resource.Barrier)
			{
				if (resource.StorageImage)
				{
					resource.StorageImage->ExecuteBarrier(currentFrame.CommandBuffer, resource.Barrier);
				}
				else if (resource.Texture)
				{
					resource.Texture->GetImage()->ExecuteBarrier(currentFrame.CommandBuffer, resource.Barrier);
				}
			}
		}*/

		for (const auto& attachment : Info.Attachments)
		{
			if (attachment.Image)
			{
				if (attachment.Image->GetImageLayout() != static_cast<VkImageLayout>(attachment.Barrier.OldLayout) && attachment.Barrier.OldLayout != ImageLayout::Undefined)
				{
					attachment.Image->ExecuteBarrier(commandBuffer, { static_cast<ImageLayout>(attachment.Image->GetImageLayout()), attachment.Barrier.OldLayout});
				}
			}
		}

		switch (Info.StageType)
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
			case RendererStageType::ScreenSpacePass:
			{
				ForwardGraphics(commandBuffer);
			}break;
			case RendererStageType::RayTracing:
			{
				RayTracing(commandBuffer);
			}break;
		}

		for (const auto& attachment : Info.Attachments)
		{
			if (attachment.Image)
			{
				attachment.Image->SetImageLayout(static_cast<VkImageLayout>(attachment.Barrier.NewLayout));
			}
		}
	}

	void RendererStage::Cleanup()
	{
		FrameBuffer.reset();
		vkDestroyRenderPass(GraphicsContext::GetDevice(), RenderPass, nullptr);
	}

	void RendererStage::ForwardGraphics(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_GPU_EVENT("ForwardGraphics Pass");
		HG_PROFILE_TAG("Name", Info.Name.c_str());

		RendererFrame& currentFrame = s_Data.GetCurrentFrame();
		VkExtent2D extent = Info.Attachments.begin()->Image->GetExtent();

		VkRenderPassBeginInfo renderPassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = RenderPass,
			.framebuffer = static_cast<VkFramebuffer>(*FrameBuffer),
			.renderArea = {
				.extent = extent
			},
			.clearValueCount = static_cast<uint32_t>(ClearValues.size()),
			.pClearValues = ClearValues.data(),
		};

		VkSubpassBeginInfo subpassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
			.contents = VK_SUBPASS_CONTENTS_INLINE,
		};

		vkCmdBeginRenderPass2(commandBuffer, &renderPassBeginInfo, &subpassBeginInfo);

		VkViewport viewport;
		viewport.x = 0.0f;
		if (Info.StageType == RendererStageType::ScreenSpacePass)
		{
			viewport.y = 0.0f;
			viewport.height = static_cast<float>(extent.height);
		}
		else
		{
			viewport.y = static_cast<float>(extent.height);
			viewport.height = -static_cast<float>(extent.height);
		}
		
		viewport.width = static_cast<float>(extent.width);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdSetDepthBias(commandBuffer, 0, 0, 0);

		Info.Pipeline->Bind(commandBuffer);

		BindResources(commandBuffer, &s_Data.GetCurrentFrame().DescriptorAllocator);

		if (Info.StageType == RendererStageType::ScreenSpacePass)
		{
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		}
		else 
		{
			for (auto && mesh : Info.Meshes)
			{
				glm::mat4 modelMat = mesh->GetModelMatrix();
				for (int i = 0; i < Info.Resources.size(); i++)
				{
					const auto& resource = Info.Resources[i];

					switch (resource.Type)
					{
						case ResourceType::PushConstant:
						{
							std::memcpy(resource.ConstantDataPointer, &modelMat, resource.ConstantSize);
							vkCmdPushConstants(commandBuffer, Info.Pipeline->GetPipelineLayout(), resource.BindLocation, 0, static_cast<uint32_t>(resource.ConstantSize), resource.ConstantDataPointer);
						}break;
						default: break;
					}
				}

				mesh->Draw(commandBuffer);
			}
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	void RendererStage::ForwardCompute(VkCommandBuffer commandBuffer)
	{
		HG_PROFILE_GPU_EVENT("ForwardCompute Pass");
		HG_PROFILE_TAG("Name", Info.Name.c_str());

		Info.Pipeline->Bind(commandBuffer);

		BindResources(commandBuffer, &s_Data.GetCurrentFrame().DescriptorAllocator);

		vkCmdDispatch(commandBuffer, Info.GroupCounts.x, Info.GroupCounts.y, Info.GroupCounts.z);
	}

	void RendererStage::ImGui(VkCommandBuffer commandBuffer)
	{
		// ImGui
		HG_PROFILE_GPU_EVENT("ImGui Pass");

		VkRenderPassBeginInfo renderPassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = RenderPass,
			.framebuffer = static_cast<VkFramebuffer>(*FrameBuffer),
			.renderArea = {
				.extent = FrameBuffer->GetExtent(),
			},
			.clearValueCount = static_cast<uint32_t>(ClearValues.size()),
			.pClearValues = ClearValues.data(),
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

	void RendererStage::BlitStage(VkCommandBuffer commandBuffer)
	{
		// Copy to final target
		HG_PROFILE_GPU_EVENT("Blit Pass");
		RendererFrame& currentFrame = s_Data.GetCurrentFrame();
		VkExtent2D extent = currentFrame.FrameBuffer->GetExtent();

		VkRenderPassBeginInfo renderPassBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = RenderPass,
			.framebuffer = static_cast<VkFramebuffer>(*(currentFrame.FrameBuffer)),
			.renderArea = {
				.extent = extent
			},
			.clearValueCount = static_cast<uint32_t>(ClearValues.size()),
			.pClearValues = ClearValues.data(),
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

		Info.Pipeline->Bind(commandBuffer);

		BindResources(commandBuffer, &currentFrame.DescriptorAllocator);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);
	}

	void RendererStage::RayTracing(VkCommandBuffer commandBuffer)
	{
		Info.Pipeline->Bind(commandBuffer);
		BindResources(commandBuffer, &s_Data.GetCurrentFrame().DescriptorAllocator);

		vkCmdTraceRaysKHR(commandBuffer,
			Info.ShaderBindingTable->GetRaygenShaderSBTEntry(),
			Info.ShaderBindingTable->GetMissShaderSBTEntry(),
			Info.ShaderBindingTable->GetHitShaderSBTEntry(),
			Info.ShaderBindingTable->GetCallableShaderSBTEntry(),
			Info.GroupCounts.x,
			Info.GroupCounts.y,
			Info.GroupCounts.z
		);
	}

	void RendererStage::BindResources(VkCommandBuffer commandBuffer, DescriptorAllocator* allocator)
	{
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		auto db = DescriptorBuilder::Begin(Renderer::GetDescriptorLayoutCache(), allocator);
		std::vector<VkDescriptorImageInfo*> imageInfos;
		std::vector<VkDescriptorBufferInfo*> bufferInfos;
		std::vector<VkWriteDescriptorSetAccelerationStructureKHR*> acceleratrionStructureInfos;

		for (int i = 0; i < Info.Resources.size(); i++)
		{
			const auto& resource = Info.Resources[i];

			switch(resource.Type)
			{
				case ResourceType::Sampler:
				{
					VkDescriptorImageInfo* sourceImage = new VkDescriptorImageInfo;
					sourceImage->sampler = resource.Texture->GetSampler();

					sourceImage->imageView = resource.Texture->GetImageView();
					sourceImage->imageLayout = resource.Texture->GetImageLayout();
					imageInfos.push_back(sourceImage);
					
					db.BindImage(resource.Binding, imageInfos.back(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resource.BindLocation);
				}break;
				case ResourceType::StorageImage:
				{
					VkDescriptorImageInfo* sourceImage = new VkDescriptorImageInfo;
					sourceImage->imageView = resource.StorageImage->GetImageView();
					sourceImage->imageLayout = resource.StorageImage->GetImageLayout();
					imageInfos.push_back(sourceImage);

					db.BindImage(resource.Binding, imageInfos.back(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, resource.BindLocation);
				}break;
				case ResourceType::Storage:
				{
					VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
					bufferInfo->buffer = resource.Buffer->GetHandle();
					bufferInfo->offset = 0;
					bufferInfo->range = VK_WHOLE_SIZE;
					bufferInfos.push_back(bufferInfo);

					db.BindBuffer(resource.Binding, bufferInfos.back(), resource.Buffer->GetBufferDescription(), resource.BindLocation);
				}break;
				case ResourceType::Uniform:
				{
					VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
					bufferInfo->buffer = resource.Buffer->GetHandle();
					bufferInfo->offset = 0;
					bufferInfo->range = VK_WHOLE_SIZE;
					bufferInfos.push_back(bufferInfo);

					db.BindBuffer(resource.Binding, bufferInfos.back(), resource.Buffer->GetBufferDescription(), resource.BindLocation);
				}break;
				case ResourceType::PushConstant: break;
				case ResourceType::Constant: break;
				case ResourceType::SamplerArray:
				{
					VkDescriptorImageInfo* imageInfosLocal = new VkDescriptorImageInfo[resource.Textures.size()];
					for (int i = 0; i < resource.Textures.size(); ++i)
					{
						imageInfosLocal[i].sampler = resource.Textures[i]->GetSampler();

						imageInfosLocal[i].imageView = resource.Textures[i]->GetImageView();
						imageInfosLocal[i].imageLayout = resource.Textures[i]->GetImageLayout();
					}

					imageInfos.push_back(imageInfosLocal);

					db.BindImage(resource.Binding, imageInfos.back(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resource.BindLocation, resource.Textures.size(), resource.ArrayMaxCount);
				}break;
				case ResourceType::AccelerationStructure:
				{
					VkWriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo = new VkWriteDescriptorSetAccelerationStructureKHR;
					accelerationStructureInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
					accelerationStructureInfo->pNext = nullptr;
					accelerationStructureInfo->accelerationStructureCount = 1;
					accelerationStructureInfo->pAccelerationStructures = resource.TLAS->GetHandlePtr();

					acceleratrionStructureInfos.push_back(accelerationStructureInfo);

					db.BindAccelerationStructure(resource.Binding, acceleratrionStructureInfos.back(), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, resource.BindLocation);
				}break;
			}
		}

		db.Build(descriptorSet);

		vkCmdBindDescriptorSets(commandBuffer, ToPipelineBindPoint(Info.StageType), Info.Pipeline->GetPipelineLayout(),
			0, 1, &descriptorSet, 0, nullptr);

		for (size_t i = 0; i < imageInfos.size(); i++)
		{
			delete imageInfos[i];
		}

		for (size_t i = 0; i < bufferInfos.size(); i++)
		{
			delete bufferInfos[i];
		}
		
		for (size_t i = 0; i < acceleratrionStructureInfos.size(); i++)
		{
			delete acceleratrionStructureInfos[i];
		}
	}
}
