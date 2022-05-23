#pragma once
#include "Hog/Renderer/Image.h"

namespace Hog {
	class FrameBuffer
	{
	public:
		FrameBuffer() = default;
		~FrameBuffer() = default;

		void Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent);
		void Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass);
		void Cleanup();

		VkExtent2D GetExtent() const { return m_Extent; }

		operator VkFramebuffer() { return m_Handle; }
	private:
		std::vector<Ref<Image>> m_Attachments;
		VkFramebuffer m_Handle = VK_NULL_HANDLE;
		VkExtent2D m_Extent = {};
		bool m_UseSwapchainExtent = false;
	};
}
