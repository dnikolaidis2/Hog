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

			VkAttachmentDescription2 colorAttachment = {
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.format = GraphicsContext::GetSwapchainFormat(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			};

			VkAttachmentReference2 colorAttachmentRef = {
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			//we are going to create 1 subpass, which is the minimum you can do
			VkSubpassDescription2 subpass = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef,
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
				//2 attachments from said array
				.attachmentCount = 1,
				.pAttachments = &colorAttachment,
				.subpassCount = 1,
				.pSubpasses = &subpass,
				//.dependencyCount = 1,
				//.pDependencies = &dependency,
			};

			CheckVkResult(vkCreateRenderPass2(GraphicsContext::GetDevice(), &renderPassInfo, nullptr, &RenderPass));
		}
	};

	struct ImGuiStageData
	{
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		FrameBuffer FrameBuffer;

		void Init()
		{
			CreateRenderPass();
		}

		void Cleanup()
		{
			vkDestroyRenderPass(GraphicsContext::GetDevice(), RenderPass, nullptr);
			FrameBuffer.Cleanup();
		}

		void CreateRenderPass()
		{
			VkAttachmentDescription2 colorAttachment = {
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.format = VK_FORMAT_B8G8R8A8_SRGB,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			VkAttachmentReference2 colorAttachmentRef = {
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			//we are going to create 1 subpass, which is the minimum you can do
			VkSubpassDescription2 subpass = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &colorAttachmentRef,
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
				.attachmentCount = 1,
				.pAttachments = &colorAttachment,
				.subpassCount = 1,
				.pSubpasses = &subpass,
				//.dependencyCount = 1,
				//.pDependencies = &dependency,
			};

			CheckVkResult(vkCreateRenderPass2(GraphicsContext::GetDevice(), &renderPassInfo, nullptr, &RenderPass));
		}
	};

	struct RendererStageData
	{
		RendererStage Info;
		VkRenderPass RenderPass = VK_NULL_HANDLE;

		void Init()
		{
			if (Info.StageType == RendererStageType::DeferredGraphics || Info.StageType == RendererStageType::ForwardGraphics)
			{
				// std::vector<VkAttachmentDescription2> attachment(Info.);

				VkAttachmentReference2 colorAttachmentRef = {
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.attachment = 0,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				};

				//we are going to create 1 subpass, which is the minimum you can do
				VkSubpassDescription2 subpass = {
					.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = 1,
					.pColorAttachments = &colorAttachmentRef,
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
					.attachmentCount = 1,
					// .pAttachments = &colorAttachment,
					.subpassCount = 1,
					.pSubpasses = &subpass,
					//.dependencyCount = 1,
					//.pDependencies = &dependency,
				};

				CheckVkResult(vkCreateRenderPass2(GraphicsContext::GetDevice(), &renderPassInfo, nullptr, &RenderPass));
			}
		}
	};

	struct FrameData
	{
		VkCommandPool CommandPool = VK_NULL_HANDLE;;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;;
		VkFence Fence = VK_NULL_HANDLE;;
		VkSemaphore PresentSemaphore = VK_NULL_HANDLE;;
		VkSemaphore RenderSemaphore = VK_NULL_HANDLE;;
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

	struct RendererData
	{
		RenderGraph Graph;
		std::vector<FrameData> FrameData;
		std::vector<RendererStageData> StageData;
		ImGuiStageData ImGuiStage;
		BlitStageData BlitStage;
		DescriptorLayoutCache DescriptorLayoutCache;
		Ref<Image> FinalTarget;
		Ref<ImGuiLayer> ImGuiLayer;

		uint32_t FrameIndex = 0;
		uint32_t MaxFrameCount = 2;
	};

	static RendererData s_Data;

	void Renderer::Initialize(RenderGraph renderGraph)
	{
		s_Data.MaxFrameCount = *CVarSystem::Get()->GetIntCVar("renderer.frameCount");
		s_Data.FrameData.resize(s_Data.MaxFrameCount);
		std::for_each(s_Data.FrameData.begin(), s_Data.FrameData.end(), [](FrameData& elem) { elem.Init(); });

		s_Data.BlitStage.Init();
		std::vector<Ref<Image>>& swapchainImages = GraphicsContext::GetSwapchainImages();
		
		for (int i = 0; i < swapchainImages.size(); ++i)
		{
			std::vector<Ref<Image>> attachments(1);
			attachments[0] = swapchainImages[i];
			s_Data.FrameData[i].FrameBuffer.Create(attachments, s_Data.BlitStage.RenderPass);
		}
		s_Data.BlitStage.Shader->Generate(s_Data.BlitStage.RenderPass, {});

		GetFinalRenderTarget();

		if (*CVarSystem::Get()->GetIntCVar("application.enableImGui"))
		{
			s_Data.ImGuiStage.Init();
			std::vector<Ref<Image>> attachments(1);
			attachments[0] = GetFinalRenderTarget();
			s_Data.ImGuiStage.FrameBuffer.Create(attachments, s_Data.ImGuiStage.RenderPass);
			s_Data.ImGuiLayer = CreateRef<ImGuiLayer>(s_Data.ImGuiStage.RenderPass);
			Application::Get().SetImGuiLayer(s_Data.ImGuiLayer);
		}

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
	}

	void Renderer::Draw()
	{
		HG_PROFILE_FUNCTION();

		auto& currentFrame = s_Data.FrameData[s_Data.FrameIndex];

		VkExtent2D swapchainExtent = GraphicsContext::GetExtent();
		VkClearValue clearValue = {
			.color = { { 0.1f, 0.1f, 0.1f, 1.0f } },
		};

		CheckVkResult(vkWaitForFences(GraphicsContext::GetDevice(), 1, &currentFrame.Fence, VK_TRUE, UINT64_MAX));
		vkResetFences(GraphicsContext::GetDevice(), 1, &currentFrame.Fence);

		currentFrame.DescriptorAllocator.ResetPools();

		vkAcquireNextImageKHR(GraphicsContext::GetDevice(), GraphicsContext::GetSwapchain(), UINT64_MAX, currentFrame.PresentSemaphore, VK_NULL_HANDLE, &s_Data.FrameIndex);

		// begin command buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		CheckVkResult(vkBeginCommandBuffer(currentFrame.CommandBuffer, &beginInfo));
		HG_PROFILE_GPU_CONTEXT(currentFrame.CommandBuffer);
		HG_PROFILE_GPU_EVENT("Begin CommandBuffer");

		for (auto& stage : s_Data.StageData)
		{
			HG_PROFILE_GPU_EVENT("Compute Pass");
			HG_PROFILE_TAG("Name", stage.Info.Name.c_str());

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
						.BindBuffer(resource.Binding, &bufferInfo, resource.Buffer->GetBufferDescription(), VK_SHADER_STAGE_COMPUTE_BIT)
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

		// ImGui
		if (*CVarSystem::Get()->GetIntCVar("application.enableImGui"))
		{
			HG_PROFILE_GPU_EVENT("ImGui Pass");
			HG_PROFILE_TAG("Name", "ImGui");
			VkRenderPassBeginInfo renderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = s_Data.ImGuiStage.RenderPass,
				.framebuffer = static_cast<VkFramebuffer>(s_Data.ImGuiStage.FrameBuffer),
				.renderArea = {
					.extent = swapchainExtent
				},
			};

			VkSubpassBeginInfo subpassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.contents = VK_SUBPASS_CONTENTS_INLINE,
			};

			vkCmdBeginRenderPass2(currentFrame.CommandBuffer, &renderPassBeginInfo, &subpassBeginInfo);

			// Imgui draw
			s_Data.ImGuiLayer->Draw(currentFrame.CommandBuffer);

			vkCmdEndRenderPass(currentFrame.CommandBuffer);
		}

		// Copy to final target
		{
			HG_PROFILE_GPU_EVENT("Blit Pass");
			HG_PROFILE_TAG("Name", "Blit");
			VkRenderPassBeginInfo renderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = s_Data.BlitStage.RenderPass,
				.framebuffer = static_cast<VkFramebuffer>(currentFrame.FrameBuffer),
				.renderArea = {
					.extent = swapchainExtent
				}
			};

			VkSubpassBeginInfo subpassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
				.contents = VK_SUBPASS_CONTENTS_INLINE,
			};

			vkCmdBeginRenderPass2(currentFrame.CommandBuffer, &renderPassBeginInfo, &subpassBeginInfo);

			VkViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapchainExtent.width);
			viewport.height = static_cast<float>(swapchainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = swapchainExtent;

			vkCmdSetViewport(currentFrame.CommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(currentFrame.CommandBuffer, 0, 1, &scissor);

			vkCmdSetDepthBias(currentFrame.CommandBuffer, 0, 0, 0);

			s_Data.BlitStage.Shader->Bind(currentFrame.CommandBuffer);

			VkDescriptorImageInfo sourceImage;
			sourceImage.sampler = s_Data.FinalTarget->GetOrCreateSampler();

			sourceImage.imageView = s_Data.FinalTarget->GetImageView();
			sourceImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorSet blitSet;
			DescriptorBuilder::Begin(&s_Data.DescriptorLayoutCache, &currentFrame.DescriptorAllocator)
				.BindImage(0, &sourceImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build(blitSet);

			vkCmdBindDescriptorSets(currentFrame.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.BlitStage.Shader->GetPipelineLayout(),
				0, 1, &blitSet, 0, nullptr);

			vkCmdDraw(currentFrame.CommandBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(currentFrame.CommandBuffer);
		}

		// end command buffer
		CheckVkResult(vkEndCommandBuffer(currentFrame.CommandBuffer));

		// Submit
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

		// Present

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &currentFrame.RenderSemaphore;

		VkSwapchainKHR swapChains[] = { GraphicsContext::GetSwapchain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &s_Data.FrameIndex;

		HG_PROFILE_GPU_FLIP(GraphicsContext::GetSwapchain());

		CheckVkResult(vkQueuePresentKHR(GraphicsContext::GetQueue(), &presentInfo));

		s_Data.FrameIndex = (s_Data.FrameIndex + 1) % s_Data.MaxFrameCount;
	}

	void Renderer::Deinitialize()
	{
		s_Data.FinalTarget.reset();
		std::for_each(s_Data.FrameData.begin(), s_Data.FrameData.end(), [](FrameData& elem) {elem.Cleanup(); });
		s_Data.FrameData.clear();
		s_Data.BlitStage.Cleanup();
		s_Data.StageData.clear();
		s_Data.DescriptorLayoutCache.Cleanup();
		s_Data.Graph.Cleanup();

		if (*CVarSystem::Get()->GetIntCVar("application.enableImGui"))
		{
			Application::Get().PopOverlay(s_Data.ImGuiLayer);
			s_Data.ImGuiLayer.reset();
			s_Data.ImGuiStage.Cleanup();
		}
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
