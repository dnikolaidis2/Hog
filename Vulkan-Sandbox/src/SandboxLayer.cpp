#include "SandboxLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanCore/Renderer/GraphicsContext.h"
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanCore/Utils/RendererUtils.h"
#include "VulkanCore/Utils/Loader.h"

static auto& context = GraphicsContext::Get();

SandboxLayer::SandboxLayer()
	: Layer("SandboxLayer")
{

}

void SandboxLayer::OnAttach()
{
	VKC_PROFILE_FUNCTION()

	context.Initialize();

	VKC_PROFILE_GPU_INIT_VULKAN(&context.Device, &context.PhysicalDevice, &context.GraphicsQueue, &context.GraphicsFamilyIndex, 1);

	// LoadObjFile("assets/models/sponza/sponza.obj", m_Meshes);
	LoadObjFile("assets/models/monkey/monkey_smooth.obj", m_Meshes);

	m_Shader = CreateRef<Shader>("assets/shaders/Basic.glsl");

	VertexBuffer vertexBuffer;
	vertexBuffer.SetLayout(Vertex::Layout);

	{
		m_Pipeline = CreateRef<GraphicsPipeline>(context.Device, m_Shader->GetPipelineLayout(), context.SwapchainExtent, context.RenderPass);

		m_Pipeline->AddShaderStage(Shader::ShaderType::Vertex, m_Shader->GetVertexShaderModule());
		m_Pipeline->AddShaderStage(Shader::ShaderType::Fragment, m_Shader->GetFragmentShaderModule());

		m_Pipeline->VertexInputBindingDescriptions.push_back(vertexBuffer.GetInputBindingDescription());
		m_Pipeline->VertexInputAttributeDescriptions = vertexBuffer.GetInputAttributeDescriptions();
		
		m_Pipeline->PipelineDepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		m_Pipeline->Create();
	}

	{
		m_UniformBuffers.resize(context.FrameCount);
		const size_t bufferSize = sizeof(UniformBufferObject);

		for (int i = 0; i < context.FrameCount; ++i)
		{
			m_UniformBuffers[i] = CreateRef<MemoryBuffer>();
			m_UniformBuffers[i]->Create(bufferSize, MemoryType::UniformBuffer);
		}
	}

	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = (uint32_t)(context.SwapchainImages.size());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = (uint32_t)(context.SwapchainImages.size());

		CheckVkResult(vkCreateDescriptorPool(context.Device, &poolInfo, nullptr, &m_DescriptorPool));
	}

	{
		std::vector<VkDescriptorSetLayout> layouts(context.FrameCount, m_Shader->GetDescriptorSetLayouts()[0]);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(context.FrameCount);
		CheckVkResult(vkAllocateDescriptorSets(context.Device, &allocInfo, m_DescriptorSets.data()));

		for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i]->Handle;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(context.Device, 1, &descriptorWrite, 0, nullptr);
		}
	}
	
	{
		auto& commandBuffers = context.CommandBuffers;
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // Optional
			beginInfo.pInheritanceInfo = nullptr; // Optional

			CheckVkResult(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = context.RenderPass;
			renderPassInfo.framebuffer = context.FrameBuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = context.SwapchainExtent;

			VkClearValue clearValues[2];
			clearValues[0] = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

			//clear depth at 1
			clearValues[1].depthStencil.depth = 1.f;

			renderPassInfo.clearValueCount = 2;
			renderPassInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_Pipeline);

			std::vector<VkBuffer> vertexBuffers(1);
			std::vector<VkDeviceSize> offsets(1);
			for (int i = 0; i < vertexBuffers.size(); ++i)
			{
				vertexBuffers[i] = m_Meshes[i].GetBufferHandle();
				offsets[i] = 0;
			}
			
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers.data(), offsets.data());

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader->GetPipelineLayout(), 0, 1, &m_DescriptorSets[i], 0, nullptr);
			vkCmdDraw(commandBuffers[i], (uint32_t)m_Meshes[0].GetSize(), 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			CheckVkResult(vkEndCommandBuffer(commandBuffers[i]));
		}
	}
}

void SandboxLayer::OnDetach()
{
	VKC_PROFILE_FUNCTION()

	vkDeviceWaitIdle(context.Device);

	for (auto& mesh : m_Meshes)
	{
		mesh.Destroy();
	}

	for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
		m_UniformBuffers[i].reset();
	}

	vkDestroyDescriptorPool(context.Device, m_DescriptorPool, nullptr);

	m_Pipeline->Destroy();

	m_Shader.reset();

	context.Deinitialize();
}

void SandboxLayer::OnUpdate(Timestep ts)
{
	VKC_PROFILE_FUNCTION()

	int currentFrame = context.CurrentFrame;
	auto& commandBufferFences = context.CommandBufferFences;
	auto& acquireSemaphores = context.AcquireSemaphores;
	auto& renderCompleteSemaphores = context.RenderCompleteSemaphores;
	auto& commandBuffers = context.CommandBuffers;

	vkWaitForFences(context.Device, 1, &commandBufferFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(context.Device, context.Swapchain, UINT64_MAX, acquireSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	UniformBufferObject ubo{};
	glm::vec3 camPos = { 0.f,0.f,-2.f };
	ubo.Model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(m_FrameNumber * .01f), glm::vec3(0, 1, 0));
	ubo.View = glm::translate(glm::mat4(1.f), camPos);
	ubo.Projection = glm::perspective(glm::radians(45.0f), (float)context.SwapchainExtent.width / (float)context.SwapchainExtent.height, 0.1f, 10.0f);
	ubo.Projection[1][1] *= -1;

	m_UniformBuffers[imageIndex]->SetData(&ubo, sizeof(ubo));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { acquireSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderCompleteSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(context.Device, 1, &commandBufferFences[currentFrame]);

	CheckVkResult(vkQueueSubmit(context.GraphicsQueue, 1, &submitInfo, commandBufferFences[currentFrame]));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { context.Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	VKC_PROFILE_GPU_FLIP(context.Swapchain);
	vkQueuePresentKHR(context.PresentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % context.FrameCount;

	context.CurrentFrame = currentFrame;
	m_FrameNumber++;
}

void SandboxLayer::OnImGuiRender()
{
}

void SandboxLayer::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<FrameBufferResizeEvent>(VKC_BIND_EVENT_FN(SandboxLayer::OnResized));
}

bool SandboxLayer::OnResized(FrameBufferResizeEvent& e)
{
	vkDeviceWaitIdle(context.Device);

	for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
		m_UniformBuffers[i].reset();
	}

	vkDestroyDescriptorPool(context.Device, m_DescriptorPool, nullptr);
	m_Pipeline->Destroy();
	GraphicsContext::RecreateSwapChain();

	m_Pipeline->Update(context.SwapchainExtent, context.RenderPass);
	m_Pipeline->Create();

	{
		m_UniformBuffers.resize(context.FrameCount);
		const size_t bufferSize = sizeof(UniformBufferObject);

		for (int i = 0; i < context.FrameCount; ++i)
		{
			m_UniformBuffers[i] = CreateRef<MemoryBuffer>();
			m_UniformBuffers[i]->Create(bufferSize, MemoryType::UniformBuffer);
		}
	}

	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = (uint32_t)(context.SwapchainImages.size());

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = (uint32_t)(context.SwapchainImages.size());

		CheckVkResult(vkCreateDescriptorPool(context.Device, &poolInfo, nullptr, &m_DescriptorPool));
	}

	{
		std::vector<VkDescriptorSetLayout> layouts(context.FrameCount, m_Shader->GetDescriptorSetLayouts()[0]);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(context.FrameCount);
		CheckVkResult(vkAllocateDescriptorSets(context.Device, &allocInfo, m_DescriptorSets.data()));

		for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i]->Handle;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(context.Device, 1, &descriptorWrite, 0, nullptr);
		}
	}

	auto& commandBuffers = context.CommandBuffers;
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		CheckVkResult(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = context.RenderPass;
		renderPassInfo.framebuffer = context.FrameBuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = context.SwapchainExtent;

		VkClearValue clearValues[2];
		clearValues[0] = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		//clear depth at 1
		clearValues[1].depthStencil.depth = 1.f;

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_Pipeline);

		std::vector<VkBuffer> vertexBuffers(1);
		std::vector<VkDeviceSize> offsets(1);
		for (int i = 0; i < vertexBuffers.size(); ++i)
		{
			vertexBuffers[i] = m_Meshes[i].GetBufferHandle();
			offsets[i] = 0;
		}

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers.data(), offsets.data());

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader->GetPipelineLayout(), 0, 1, &m_DescriptorSets[i], 0, nullptr);
		vkCmdDraw(commandBuffers[i], (uint32_t)m_Meshes[0].GetSize(), 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		CheckVkResult(vkEndCommandBuffer(commandBuffers[i]));
	}

	return false;
}