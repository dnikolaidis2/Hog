#pragma once
#include "Hog/Renderer/Resource/Image.h"

namespace Hog {
	class FrameBuffer
	{
	public:
		static Ref<FrameBuffer> Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent);
		static Ref<FrameBuffer> Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass);
	public:
		FrameBuffer(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent);
		~FrameBuffer();

		VkExtent2D GetExtent() const { return m_Extent; }

		operator VkFramebuffer() { return m_Handle; }
	private:
		std::vector<Ref<Image>> m_Attachments;
		VkFramebuffer m_Handle = VK_NULL_HANDLE;
		VkExtent2D m_Extent = {};
	};
}
