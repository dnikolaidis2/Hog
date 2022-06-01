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
	EditorCamera m_EditorCamera;
	std::vector<Ref<Mesh>> m_Meshes;
	std::vector<glm::mat4> m_Cameras;
	Ref<Buffer> m_ViewProjection;
	PushConstant m_PushConstant;
};
