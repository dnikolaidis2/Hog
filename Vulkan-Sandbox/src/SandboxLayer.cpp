#include "SandboxLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanCore/Renderer/GraphicsContext.h"

SandboxLayer::SandboxLayer()
	: Layer("SandboxLayer")
{
}

void SandboxLayer::OnAttach()
{
	VulkanCore::GraphicsContext::Initialize();
}

void SandboxLayer::OnDetach()
{
	VulkanCore::GraphicsContext::Deinitialize();
}

void SandboxLayer::OnUpdate(VulkanCore::Timestep ts)
{
}

void SandboxLayer::OnImGuiRender()
{
}

void SandboxLayer::OnEvent(VulkanCore::Event& e)
{
}
