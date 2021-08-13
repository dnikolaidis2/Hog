#pragma once

#include "VulkanCore.h"

class SandboxLayer : public VulkanCore::Layer
{
public:
	SandboxLayer();
	virtual ~SandboxLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(VulkanCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(VulkanCore::Event& e) override;
private:
};