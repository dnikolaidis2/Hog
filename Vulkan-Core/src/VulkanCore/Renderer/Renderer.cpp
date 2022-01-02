#include "vkcpch.h"
#include "Renderer.h"

#include "VulkanCore/Renderer/GraphicsPipeline.h"
#include "VulkanCore/Renderer/GraphicsContext.h"
#include "VulkanCore/Utils/RendererUtils.h"

namespace VulkanCore
{
	struct CameraData {
		glm::mat4 ViewProjection;
	};

	struct ModelPushConstant
	{
		glm::mat4 Model;
	};

	struct RendererStats
	{
		uint64_t FrameCount = 0;
	};

	struct RendererState
	{
		VkDevice Device;
		int MaxFrameCount;

		std::vector<CameraData> CameraBuffers;
		std::vector<Ref<Buffer>> CameraUniformBuffers;
		std::vector<std::array<MaterialGPUData, MATERIAL_ARRAY_SIZE>> MaterialBuffers;
		std::vector<Ref<Buffer>> MaterialUniformBuffers;
		std::array<VkDescriptorImageInfo, TEXTURE_ARRAY_SIZE> TextureDescriptorImageInfos;

		Ref<GraphicsPipeline> BoundPipeline = nullptr;

		VkDescriptorSetLayoutBinding GlobalDescriptorSetLayoutBinding =
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		};
		VkDescriptorSetLayout GlobalDescriptorLayout;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSet> GlobalDescriptorSets;

		RendererStats Statistics;

		VkCommandBuffer CurrentCommandBuffer;
		VkSemaphore CurrentAcquireSemaphore;
		VkSemaphore CurrentRenderCompleteSemaphore;
		VkFence CurentCommandBufferFence;
		VkDescriptorSet* CurrentGlobalDescriptorSetPtr;
		uint32_t CurrentImageIndex;
	};

	static RendererState s_Data;

	void Renderer::Initialize()
	{
		s_Data.Device = GraphicsContext::GetDevice();
		s_Data.MaxFrameCount = GraphicsContext::GetFrameCount();

		Ref<Shader> basicShader = ShaderLibrary::Get("Basic");

		{
			s_Data.CameraBuffers.resize(s_Data.MaxFrameCount);
			s_Data.CameraUniformBuffers.resize(s_Data.MaxFrameCount);

			for (int i = 0; i < s_Data.MaxFrameCount; ++i)
			{
				s_Data.CameraUniformBuffers[i] = CreateRef<Buffer>(MemoryType::UniformBuffer, (uint32_t)sizeof(CameraData));
			}
		}

		{
			s_Data.MaterialBuffers.resize(s_Data.MaxFrameCount);
			s_Data.MaterialUniformBuffers.resize(s_Data.MaxFrameCount);

			for (int i = 0; i < s_Data.MaxFrameCount; ++i)
			{
				s_Data.MaterialUniformBuffers[i] = CreateRef<Buffer>(MemoryType::UniformBuffer, (uint32_t)(sizeof(MaterialGPUData) * MATERIAL_ARRAY_SIZE));
			}
		}

		{
			std::vector<VkDescriptorPoolSize> poolSize = {
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
			};

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = (uint32_t)poolSize.size();
			poolInfo.pPoolSizes = poolSize.data();
			poolInfo.maxSets = 2000;

			CheckVkResult(vkCreateDescriptorPool(s_Data.Device, &poolInfo, nullptr, &s_Data.DescriptorPool));
		}

		{
			std::vector<VkDescriptorSetLayout> layouts(s_Data.MaxFrameCount, basicShader->GetDescriptorSetLayouts()[0]);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = s_Data.DescriptorPool;
			allocInfo.descriptorSetCount = (uint32_t)layouts.size();
			allocInfo.pSetLayouts = layouts.data();

			s_Data.GlobalDescriptorSets.resize(s_Data.MaxFrameCount);
			CheckVkResult(vkAllocateDescriptorSets(s_Data.Device, &allocInfo, s_Data.GlobalDescriptorSets.data()));

			for (size_t i = 0; i < s_Data.MaxFrameCount; i++) {
				{
					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.buffer = s_Data.CameraUniformBuffers[i]->GetHandle();
					bufferInfo.offset = 0;
					bufferInfo.range = sizeof(CameraData);

					VkWriteDescriptorSet descriptorWrite{};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = s_Data.GlobalDescriptorSets[i];
					descriptorWrite.dstBinding = 0;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pBufferInfo = &bufferInfo;
					descriptorWrite.pImageInfo = nullptr; // Optional
					descriptorWrite.pTexelBufferView = nullptr; // Optional

					vkUpdateDescriptorSets(s_Data.Device, 1, &descriptorWrite, 0, nullptr);
				}

				{
					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.buffer = s_Data.MaterialUniformBuffers[i]->GetHandle();
					bufferInfo.offset = 0;
					bufferInfo.range = sizeof(MaterialGPUData) * MATERIAL_ARRAY_SIZE;

					VkWriteDescriptorSet descriptorWrite{};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = s_Data.GlobalDescriptorSets[i];
					descriptorWrite.dstBinding = 1;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pBufferInfo = &bufferInfo;
					descriptorWrite.pImageInfo = nullptr; // Optional
					descriptorWrite.pTexelBufferView = nullptr; // Optional

					vkUpdateDescriptorSets(s_Data.Device, 1, &descriptorWrite, 0, nullptr);
				}
			}
		}
	}

	void Renderer::BeginScene(const EditorCamera& camera)
	{
		VKC_PROFILE_FUNCTION();

		s_Data.BoundPipeline = nullptr;

		auto frameNumber = GraphicsContext::GetCurrentFrame();
		s_Data.CameraBuffers[frameNumber].ViewProjection = camera.GetViewProjection();
		s_Data.CameraUniformBuffers[frameNumber]->SetData(&s_Data.CameraBuffers[frameNumber], sizeof(CameraData));

		s_Data.MaterialBuffers[frameNumber] = MaterialLibrary::GetGPUArray();
		s_Data.MaterialUniformBuffers[frameNumber]->SetData(s_Data.MaterialBuffers[frameNumber].data(), sizeof(MaterialGPUData) * MATERIAL_ARRAY_SIZE);

		s_Data.CurrentCommandBuffer = GraphicsContext::GetCurrentCommandBuffer();
		s_Data.CurrentGlobalDescriptorSetPtr = &(s_Data.GlobalDescriptorSets[frameNumber]);
		s_Data.CurentCommandBufferFence = GraphicsContext::GetCurrentCommandBufferFence();
		s_Data.CurrentAcquireSemaphore = GraphicsContext::GetCurrentAcquireSemaphore();
		s_Data.CurrentRenderCompleteSemaphore = GraphicsContext::GetCurrentRenderCompleteSemaphore();

		vkWaitForFences(s_Data.Device, 1, &s_Data.CurentCommandBufferFence, VK_TRUE, UINT64_MAX);
		vkAcquireNextImageKHR(s_Data.Device, GraphicsContext::GetSwapchain(), UINT64_MAX, s_Data.CurrentAcquireSemaphore, VK_NULL_HANDLE, &s_Data.CurrentImageIndex);

		// VKC_PROFILE_GPU_CONTEXT(commandBuffers[imageIndex]);

		{
			auto arr = TextureLibrary::GetLibraryArray();
			for (int i = 0; i < TEXTURE_ARRAY_SIZE; ++i)
			{
				if (arr[i])
				{
					s_Data.TextureDescriptorImageInfos[i].sampler = arr[i]->GetOrCreateSampler();
					s_Data.TextureDescriptorImageInfos[i].imageView = arr[i]->GetImageView();
					s_Data.TextureDescriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				else
				{
					s_Data.TextureDescriptorImageInfos[i].sampler = arr[0]->GetOrCreateSampler();
					s_Data.TextureDescriptorImageInfos[i].imageView = arr[0]->GetImageView();
					s_Data.TextureDescriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
			}

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstBinding = 2;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.dstSet = s_Data.GlobalDescriptorSets[frameNumber];
			writeDescriptorSet.descriptorCount = TEXTURE_ARRAY_SIZE;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = s_Data.TextureDescriptorImageInfos.data();

			vkUpdateDescriptorSets(s_Data.Device, 1, &writeDescriptorSet, 0, nullptr);
		}

		VKC_PROFILE_SCOPE("Recording to command buffer")

		VkCommandBufferBeginInfo beginInfo {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		CheckVkResult(vkBeginCommandBuffer(s_Data.CurrentCommandBuffer, &beginInfo));

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = GraphicsContext::GetRenderPass();
		renderPassInfo.framebuffer = GraphicsContext::GetCurrentFrameBuffer();

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = GraphicsContext::GetExtent();

		VkClearValue clearValues[2];
		clearValues[0] = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		//clear depth at 1
		clearValues[1].depthStencil.depth = 1.f;

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(s_Data.CurrentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void Renderer::EndScene()
	{
		VKC_PROFILE_FUNCTION();

		vkCmdEndRenderPass(s_Data.CurrentCommandBuffer);

		CheckVkResult(vkEndCommandBuffer(s_Data.CurrentCommandBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { s_Data.CurrentAcquireSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &s_Data.CurrentCommandBuffer;

		VkSemaphore signalSemaphores[] = { s_Data.CurrentRenderCompleteSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(s_Data.Device, 1, &s_Data.CurentCommandBufferFence);

		CheckVkResult(vkQueueSubmit(GraphicsContext::GetGraphicsQueue(), 1, &submitInfo, s_Data.CurentCommandBufferFence));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { GraphicsContext::GetSwapchain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &s_Data.CurrentImageIndex;

		CheckVkResult(vkQueuePresentKHR(GraphicsContext::GetGraphicsQueue(), &presentInfo));

		int currentFrame = (GraphicsContext::GetCurrentFrame() + 1) % s_Data.MaxFrameCount;

		GraphicsContext::SetCurrentFrame(currentFrame);
		s_Data.Statistics.FrameCount++;
	}

	void Renderer::Deinitialize()
	{
		vkDeviceWaitIdle(s_Data.Device);

		s_Data.BoundPipeline = nullptr;
		s_Data.CameraUniformBuffers.clear();
		s_Data.MaterialUniformBuffers.clear();

		vkDestroyDescriptorSetLayout(s_Data.Device, s_Data.GlobalDescriptorLayout, nullptr);
		vkDestroyDescriptorPool(s_Data.Device, s_Data.DescriptorPool, nullptr);
	}

	void Renderer::DrawObject(const Ref<RendererObject> object)
	{
		VKC_PROFILE_FUNCTION();

		Ref<Material> mat = object->GetMaterial();
		Ref<Shader> shader = mat->GetShader();
		Ref<GraphicsPipeline> pipeline = mat->GetPipeline();

		ModelPushConstant constants = { .Model = object->GetTransform() };
		vkCmdPushConstants(s_Data.CurrentCommandBuffer, shader->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPushConstant), &constants);

		if (pipeline != s_Data.BoundPipeline)
		{
			pipeline->Bind(s_Data.CurrentCommandBuffer);
			vkCmdBindDescriptorSets(s_Data.CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->GetPipelineLayout(), 0, 1, s_Data.CurrentGlobalDescriptorSetPtr, 0, nullptr);

			s_Data.BoundPipeline = pipeline;
		}

		object->Draw(s_Data.CurrentCommandBuffer);
	}

	void Renderer::DrawObjects(const std::vector<Ref<RendererObject>>& objects)
	{
		VKC_PROFILE_FUNCTION();
		for (const auto& obj : objects)
		{
			DrawObject(obj);
		}
	}
}
