#include "hgpch.h"
#include "FrameBuffer.h"

#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Utils/RendererUtils.h"


namespace Hog
{
	Ref<FrameBuffer> FrameBuffer::Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent)
	{
		return CreateRef<FrameBuffer>(attachments, renderPass, extent);
	}

	Ref<FrameBuffer> FrameBuffer::Create(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass)
	{
		return Create(attachments, renderPass, GraphicsContext::GetExtent());
	}

	FrameBuffer::FrameBuffer(std::vector<Ref<Image>>& attachments, VkRenderPass renderPass, VkExtent2D extent)
		: m_Attachments(attachments), m_Extent(extent)
	{
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
			.width = m_Extent.width,
			.height = m_Extent.height,
			.layers = 1,
		};

		CheckVkResult(vkCreateFramebuffer(GraphicsContext::GetDevice(), &framebufferCreateInfo, nullptr, &m_Handle));
	}

	FrameBuffer::~FrameBuffer()
	{
		vkDestroyFramebuffer(GraphicsContext::GetDevice(), m_Handle, nullptr);
		m_Attachments.clear();
	}
}
