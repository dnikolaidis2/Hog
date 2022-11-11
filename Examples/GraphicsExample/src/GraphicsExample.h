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
	std::vector<Ref<Mesh>> m_TransparentMeshes;
	std::vector<Ref<Mesh>> m_OpaqueMeshes;
	std::vector<Ref<Texture>> m_Textures;
	std::unordered_map<std::string, Camera> m_Cameras;
	std::vector<Ref<Material>> m_Materials;
	std::vector<Ref<Light>> m_Lights;
	Ref<Buffer> m_MaterialBuffer;
	Ref<Buffer> m_ViewProjection;
	Ref<Buffer> m_LightBuffer;
	PushConstant m_PushConstant;
};
