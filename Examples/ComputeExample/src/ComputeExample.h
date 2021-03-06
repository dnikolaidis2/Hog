#pragma once

#include "Hog.h"

using namespace Hog;

class ComputeExample : public Layer
{
public:
	ComputeExample();
	virtual ~ComputeExample() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;
private:
	Ref<Buffer> m_ComputeBuffer = nullptr;	
};
