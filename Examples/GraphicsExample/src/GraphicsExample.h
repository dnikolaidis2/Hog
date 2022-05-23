#pragma once

#include "Hog.h"

using namespace Hog;

class GraphicsExample : public Layer
{
public:
	struct PushConstant
	{
		glm::mat4 Model;
	};

	GraphicsExample();
	virtual ~GraphicsExample() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Event& e) override;
	bool OnResized(FrameBufferResizeEvent& e);
private:
	std::vector<Ref<RendererObject>> m_Objects;

	EditorCamera m_EditorCamera;

	Ref<ImGuiLayer> m_ImGuiLayer;

	PushConstant m_PushConstant;
};