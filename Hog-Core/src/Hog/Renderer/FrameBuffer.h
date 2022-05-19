#pragma once
#include "Image.h"


namespace Hog {
	class FrameBuffer
	{
	public:
		FrameBuffer() = default;
		~FrameBuffer() = default;

		void Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent);
		void Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass);
		void Cleanup();

		operator VkFramebuffer() { return m_Handle; }
	private:
		std::vector<Ref<Image>> m_Attachments;
		VkFramebuffer m_Handle = VK_NULL_HANDLE;
		bool m_UseSwapchainExtent = false;
	};
}
