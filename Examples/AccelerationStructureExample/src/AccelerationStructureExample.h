#pragma once

#include "Hog.h"

using namespace Hog;

class AccelerationStructureExample : public Layer
{
public:
	AccelerationStructureExample();
	virtual ~AccelerationStructureExample() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;
private:
	std::vector<Ref<Mesh>> m_TransparentMeshes;
	std::vector<Ref<Mesh>> m_OpaqueMeshes;
	std::vector<Ref<Texture>> m_Textures;
	std::unordered_map<std::string, glm::mat4> m_Cameras;
	std::vector<Ref<Material>> m_Materials;
	std::vector<Ref<Light>> m_Lights;
	Ref<Buffer> m_MaterialBuffer;
	Ref<Buffer> m_ViewProjection;
	Ref<Buffer> m_LightBuffer;
	Ref<AccelerationStructure> m_TopLevelAS;
};
