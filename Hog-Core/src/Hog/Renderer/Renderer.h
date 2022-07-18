#pragma once

#include "Hog/Renderer/Image.h"
#include "Hog/Renderer/RenderGraph.h"
#include "Hog/Renderer/FrameBuffer.h"
#include "Hog/Renderer/Descriptor.h"
#include "Hog/Renderer/GraphicsContext.h"

namespace Hog
{
	class Renderer
	{
	public:
		static void Initialize(RenderGraph renderGraph);
		static void Cleanup();
		static DescriptorLayoutCache* GetDescriptorLayoutCache();
		static void Draw();

		struct RendererStats
		{
			uint64_t FrameCount = 0;
		};

		static RendererStats GetStats();
	};

	class RendererFrame
	{
	public:
		void Init();
		void Init(Ref<Image> swapchainImage, VkRenderPass renderPass);
		void BeginFrame();
		void EndFrame();
		void Cleanup();
	public:
		VkDevice Device = VK_NULL_HANDLE;
		VkQueue Queue = VK_NULL_HANDLE;
		VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		VkFence Fence = VK_NULL_HANDLE;
		VkSemaphore PresentSemaphore = VK_NULL_HANDLE;
		VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
		Ref<FrameBuffer> FrameBuffer;
		DescriptorAllocator DescriptorAllocator;
		Ref<Image> SwapchainImage;
	};

	class RendererStage
	{
	public:
		void Init();
		void Execute(VkCommandBuffer commandBuffer);
		void Cleanup();
	public:
		StageDescription Info;
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		Ref<FrameBuffer> FrameBuffer;
		std::vector<VkClearValue> ClearValues;
	private:
		void ForwardGraphics(VkCommandBuffer commandBuffer);
		void ForwardCompute(VkCommandBuffer commandBuffer);
		void ImGui(VkCommandBuffer commandBuffer);
		void BlitStage(VkCommandBuffer commandBuffer);
		void RayTracing(VkCommandBuffer commandBuffer);

		void BindResources(VkCommandBuffer commandBuffer, DescriptorAllocator* allocator);
	};
}
