#include "SandboxLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanCore/Renderer/GraphicsContext.h"
#include "VulkanCore/Renderer/Shader.h"
#include "VulkanCore/Utils/RendererUtils.h"

static auto& context = GraphicsContext::Get();

SandboxLayer::SandboxLayer()
	: Layer("SandboxLayer")
{

}

void SandboxLayer::OnAttach()
{
	VKC_PROFILE_FUNCTION()

	context.Initialize();
	
	m_Shader = CreateRef<Shader>("assets/shaders/Basic.glsl");

	m_VertexBuffer = CreateRef<VertexBuffer>();
	m_VertexBuffer->SetLayout(Vertex::Layout);

	{
		m_Pipeline = CreateRef<GraphicsPipeline>(context.Device, context.SwapchainExtent, context.RenderPass);

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		CheckVkResult(vkCreateDescriptorSetLayout(context.Device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

		m_Pipeline->AddShaderStage(Shader::ShaderType::Vertex, m_Shader->GetVertexShaderModule());
		m_Pipeline->AddShaderStage(Shader::ShaderType::Fragment, m_Shader->GetFragmentShaderModule());

		m_Pipeline->VertexInputBindingDescriptions.push_back(m_VertexBuffer->GetInputBindingDescription());
		m_Pipeline->VertexInputAttributeDescriptions = m_VertexBuffer->GetInputAttributeDescriptions();

		m_Pipeline->PipelineLayoutCreateInfo.setLayoutCount = 1; // Optional
		m_Pipeline->PipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional

		m_Pipeline->Create();
	}

	{
		const size_t bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();
		m_VertexBuffer->Create((uint64_t)bufferSize);

		m_VertexBuffer->SetData((void*)m_Vertices.data(), bufferSize);
	}

	{
		m_UniformBuffers.resize(context.FrameCount);
		const size_t bufferSize = sizeof(UniformBufferObject);

		for (int i = 0; i < context.FrameCount; ++i)
		{
			m_UniformBuffers[i] = CreateRef<MemoryBuffer>();
			m_UniformBuffers[i]->Create(bufferSize, MemoryUsage::UniformBuffer, MemoryType::CPUToGPU, MemorySharing::Exclusive);
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
		std::vector<VkDescriptorSetLayout> layouts(context.FrameCount, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(context.FrameCount);
		CheckVkResult(vkAllocateDescriptorSets(context.Device, &allocInfo, m_DescriptorSets.data()));

		for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = *m_UniformBuffers[i];
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
		const size_t bufferSize = sizeof(m_Indices[0]) * m_Indices.size();
		m_IndexBuffer = CreateRef<IndexBuffer>();
		m_IndexBuffer->Create(bufferSize);
		m_IndexBuffer->SetData((void*)m_Indices.data(), bufferSize);
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

			VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_Pipeline);

			VkBuffer vertexBuffers[] = { *m_VertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], *m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 0, 1, &m_DescriptorSets[i], 0, nullptr);
			vkCmdDrawIndexed(commandBuffers[i], (uint32_t)(m_Indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			CheckVkResult(vkEndCommandBuffer(commandBuffers[i]));
		}
	}
}

void SandboxLayer::OnDetach()
{
	VKC_PROFILE_FUNCTION()

	vkDeviceWaitIdle(context.Device);

	for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
		m_UniformBuffers[i].reset();
	}

	m_VertexBuffer.reset();
	m_IndexBuffer.reset();

	vkDestroyDescriptorPool(context.Device, m_DescriptorPool, nullptr);

	m_Pipeline->Destroy();

	vkDestroyDescriptorSetLayout(context.Device, m_DescriptorSetLayout, nullptr);

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
	ubo.model = glm::rotate(glm::mat4(1.0f), ts * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), (float)context.SwapchainExtent.width / (float)context.SwapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

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

	vkQueuePresentKHR(context.PresentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % context.FrameCount;

	context.CurrentFrame = currentFrame;
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
			m_UniformBuffers[i]->Create(bufferSize, MemoryUsage::UniformBuffer, MemoryType::CPUToGPU, MemorySharing::Exclusive);
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
		std::vector<VkDescriptorSetLayout> layouts(context.FrameCount, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(context.FrameCount);
		CheckVkResult(vkAllocateDescriptorSets(context.Device, &allocInfo, m_DescriptorSets.data()));

		for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = *m_UniformBuffers[i];
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

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_Pipeline);

		VkBuffer vertexBuffers[] = { *m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], *m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 0, 1, &m_DescriptorSets[i], 0, nullptr);
		vkCmdDrawIndexed(commandBuffers[i], (uint32_t)(m_Indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		CheckVkResult(vkEndCommandBuffer(commandBuffers[i]));
	}

	return false;
}