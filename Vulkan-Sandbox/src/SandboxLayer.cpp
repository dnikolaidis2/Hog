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

		CheckVKResult(vkCreateDescriptorSetLayout(context.Device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

		m_Pipeline->AddShaderStage(Shader::ShaderType::Vertex, m_Shader->GetVertexShaderModule());
		m_Pipeline->AddShaderStage(Shader::ShaderType::Fragment, m_Shader->GetFragmentShaderModule());

		m_Pipeline->VertexInputBindingDescriptions.push_back(Vertex::GetBindingDescription());
		m_Pipeline->VertexInputAttributeDescriptions = Vertex::GetAttributeDescriptions();

		m_Pipeline->PipelineLayoutCreateInfo.setLayoutCount = 1; // Optional
		m_Pipeline->PipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional

		m_Pipeline->Create();
	}

	{
		const size_t bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = (uint32_t)bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;

		CheckVKResult(vmaCreateBuffer(context.Allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, nullptr));

		void* data;
		vmaMapMemory(context.Allocator, stagingAllocation, &data);
		memcpy(data, m_Vertices.data(), bufferSize);
		vmaUnmapMemory(context.Allocator, stagingAllocation);


		//allocate vertex buffer
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;
		//this is the total size, in bytes, of the buffer we are allocating
		vertexBufferInfo.size = (uint32_t)bufferSize;
		//this buffer is going to be used as a Vertex Buffer
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//let the VMA library know that this data should be GPU native
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		//allocate the buffer
		CheckVKResult(vmaCreateBuffer(context.Allocator, &vertexBufferInfo, &allocInfo, &m_VertexBuffer, &m_VertexBufferAllocation, nullptr));


		VkCommandBufferAllocateInfo bufferAllocationInfo{};
		bufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocationInfo.commandPool = context.CommandPool;
		bufferAllocationInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(context.Device, &bufferAllocationInfo, &commandBuffer);

		// We create fences that we can use to wait for a 
		// given command buffer to be done on the GPU.
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence uploadFence;
		CheckVKResult(vkCreateFence(context.Device, &fenceCreateInfo, nullptr, &uploadFence));

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_VertexBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(context.GraphicsQueue, 1, &submitInfo, uploadFence);

		vkWaitForFences(context.Device, 1, &uploadFence, true, UINT64_MAX);
		vkResetFences(context.Device, 1, &uploadFence);

		vkFreeCommandBuffers(context.Device, context.CommandPool, 1, &commandBuffer);
		vkDestroyFence(context.Device, uploadFence, nullptr);

		vmaDestroyBuffer(context.Allocator, stagingBuffer, stagingAllocation);
	}

	{
		m_UniformBuffers.resize(context.FrameCount);
		m_UniformBuffersAllocations.resize(context.FrameCount);
		const size_t bufferSize = sizeof(UniformBufferObject);

		for (int i = 0; i < context.FrameCount; ++i)
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = (uint32_t)bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

			CheckVKResult(vmaCreateBuffer(context.Allocator, &bufferInfo, &allocInfo, &m_UniformBuffers[i], &m_UniformBuffersAllocations[i], nullptr));
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

		CheckVKResult(vkCreateDescriptorPool(context.Device, &poolInfo, nullptr, &m_DescriptorPool));
	}

	{
		std::vector<VkDescriptorSetLayout> layouts(context.FrameCount, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(context.FrameCount);
		CheckVKResult(vkAllocateDescriptorSets(context.Device, &allocInfo, m_DescriptorSets.data()));

		for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
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
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = (uint32_t)bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;

		CheckVKResult(vmaCreateBuffer(context.Allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, nullptr));

		void* data;
		vmaMapMemory(context.Allocator, stagingAllocation, &data);
		memcpy(data, m_Indices.data(), (size_t)bufferSize);
		vmaUnmapMemory(context.Allocator, stagingAllocation);


		//allocate vertex buffer
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;
		//this is the total size, in bytes, of the buffer we are allocating
		vertexBufferInfo.size = (uint32_t)bufferSize;
		//this buffer is going to be used as a Vertex Buffer
		vertexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//let the VMA library know that this data should be GPU native
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		//allocate the buffer
		CheckVKResult(vmaCreateBuffer(context.Allocator, &vertexBufferInfo, &allocInfo, &m_IndexBuffer, &m_IndexBufferAllocation, nullptr));


		VkCommandBufferAllocateInfo bufferAllocationInfo{};
		bufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocationInfo.commandPool = context.CommandPool;
		bufferAllocationInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(context.Device, &bufferAllocationInfo, &commandBuffer);

		// We create fences that we can use to wait for a 
		// given command buffer to be done on the GPU.
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence uploadFence;
		CheckVKResult(vkCreateFence(context.Device, &fenceCreateInfo, nullptr, &uploadFence));

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_IndexBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(context.GraphicsQueue, 1, &submitInfo, uploadFence);

		vkWaitForFences(context.Device, 1, &uploadFence, true, UINT64_MAX);
		vkResetFences(context.Device, 1, &uploadFence);

		vkFreeCommandBuffers(context.Device, context.CommandPool, 1, &commandBuffer);
		vkDestroyFence(context.Device, uploadFence, nullptr);

		vmaDestroyBuffer(context.Allocator, stagingBuffer, stagingAllocation);
	}

	{
		auto& commandBuffers = context.CommandBuffers;
		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // Optional
			beginInfo.pInheritanceInfo = nullptr; // Optional

			CheckVKResult(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

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

			VkBuffer vertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 0, 1, &m_DescriptorSets[i], 0, nullptr);
			vkCmdDrawIndexed(commandBuffers[i], (uint32_t)(m_Indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			CheckVKResult(vkEndCommandBuffer(commandBuffers[i]));
		}
	}
}

void SandboxLayer::OnDetach()
{
	VKC_PROFILE_FUNCTION()

	vkDeviceWaitIdle(context.Device);

	for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
		vmaDestroyBuffer(context.Allocator, m_UniformBuffers[i], m_UniformBuffersAllocations[i]);
	}

	vmaDestroyBuffer(context.Allocator, m_VertexBuffer, m_VertexBufferAllocation);
	vmaDestroyBuffer(context.Allocator, m_IndexBuffer, m_IndexBufferAllocation);

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

	void* data;
	vmaMapMemory(context.Allocator, m_UniformBuffersAllocations[imageIndex], &data);
		memcpy(data, &ubo, sizeof(ubo));
	vmaUnmapMemory(context.Allocator, m_UniformBuffersAllocations[imageIndex]);

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

	CheckVKResult(vkQueueSubmit(context.GraphicsQueue, 1, &submitInfo, commandBufferFences[currentFrame]));

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
		vmaDestroyBuffer(context.Allocator, m_UniformBuffers[i], m_UniformBuffersAllocations[i]);
	}

	vkDestroyDescriptorPool(context.Device, m_DescriptorPool, nullptr);
	m_Pipeline->Destroy();
	GraphicsContext::RecreateSwapChain();

	m_Pipeline->Update(context.SwapchainExtent, context.RenderPass);
	m_Pipeline->Create();

	{
		m_UniformBuffers.resize(context.FrameCount);
		m_UniformBuffersAllocations.resize(context.FrameCount);
		const size_t bufferSize = sizeof(UniformBufferObject);

		for (int i = 0; i < context.FrameCount; ++i)
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = (uint32_t)bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

			CheckVKResult(vmaCreateBuffer(context.Allocator, &bufferInfo, &allocInfo, &m_UniformBuffers[i], &m_UniformBuffersAllocations[i], nullptr));
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

		CheckVKResult(vkCreateDescriptorPool(context.Device, &poolInfo, nullptr, &m_DescriptorPool));
	}

	{
		std::vector<VkDescriptorSetLayout> layouts(context.FrameCount, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(context.FrameCount);
		CheckVKResult(vkAllocateDescriptorSets(context.Device, &allocInfo, m_DescriptorSets.data()));

		for (size_t i = 0; i < m_UniformBuffers.size(); i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
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

		CheckVKResult(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

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

		VkBuffer vertexBuffers[] = { m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 0, 1, &m_DescriptorSets[i], 0, nullptr);
		vkCmdDrawIndexed(commandBuffers[i], (uint32_t)(m_Indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		CheckVKResult(vkEndCommandBuffer(commandBuffers[i]));
	}

	return false;
}
