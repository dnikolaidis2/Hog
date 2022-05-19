#include "hgpch.h"
#include "FrameBuffer.h"

#include "GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"


namespace Hog
{
	void FrameBuffer::Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent)
	{
		m_Attachments = attachments;
		std::vector<VkImageView> views(m_Attachments.size());
		for (int i = 0; i < m_Attachments.size(); ++i)
		{
			views[i] = m_Attachments[i]->GetImageView();
		}

		VkFramebufferCreateInfo framebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = extent.width,
			.height = extent.height,
			.layers = 1,
		};

		CheckVkResult(vkCreateFramebuffer(GraphicsContext::GetDevice(), &framebufferCreateInfo, nullptr, &m_Handle));
	}

	void FrameBuffer::Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass)
	{
		m_Attachments = attachments;
		m_UseSwapchainExtent = true;
		std::vector<VkImageView> views(m_Attachments.size());
		for (int i = 0; i < m_Attachments.size(); ++i)
		{
			views[i] = m_Attachments[i]->GetImageView();
		}

		VkExtent2D extent = GraphicsContext::GetExtent();

		VkFramebufferCreateInfo framebufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = extent.width,
			.height = extent.height,
			.layers = 1,
		};

		CheckVkResult(vkCreateFramebuffer(GraphicsContext::GetDevice(), &framebufferCreateInfo, nullptr, &m_Handle));
	}

	void FrameBuffer::Cleanup()
	{
		vkDestroyFramebuffer(GraphicsContext::GetDevice(), m_Handle, nullptr);
		m_Attachments.clear();
	}
}
