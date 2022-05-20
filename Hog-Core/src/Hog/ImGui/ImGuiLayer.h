#pragma once

#include "Hog/Core/Layer.h"

#include "Hog/Events/ApplicationEvent.h"
#include "Hog/Events/KeyEvent.h"
#include "Hog/Events/MouseEvent.h"

namespace Hog {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer(VkRenderPass renderPass);
		~ImGuiLayer() = default;

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Draw(VkCommandBuffer commandBuffer);

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }
		
		void SetDarkThemeColors();
	private:
		bool m_BlockEvents = true;
		VkRenderPass m_RenderPass;
	};

}