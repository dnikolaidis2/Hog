#include "SandboxLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <VulkanCore/Utils/Loader.h>

static auto& context = GraphicsContext::Get();

SandboxLayer::SandboxLayer()
	: Layer("SandboxLayer")
{

}

void SandboxLayer::OnAttach()
{
	VKC_PROFILE_FUNCTION()

	context.Initialize();

	ShaderLibrary::LoadDirectory("assets/shaders");

	LoadObjFile("assets/models/sponza/sponza.obj", m_Objects);
	// LoadObjFile("assets/models/monkey/monkey_flat.obj", m_Objects);

	// VKC_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.GraphicsQueue), &(context.GraphicsFamilyIndex), 1)

	for (auto & obj : m_Objects)
	{
		obj->SetTransform(glm::rotate(glm::mat4{ 1.0f }, glm::radians(90.0f), glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4{ 1.0f }, glm::radians(180.0f), glm::vec3(1, 0, 0)));
	}

	Renderer::Inititalize();

	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 10000.0f);
}

void SandboxLayer::OnDetach()
{
	VKC_PROFILE_FUNCTION()

	Renderer::Deinitialize();
	ShaderLibrary::Deinitialize();
	MaterialLibrary::Deinitialize();

	m_Objects.clear();

	GraphicsContext::Deinitialize();
}

void SandboxLayer::OnUpdate(Timestep ts)
{
	VKC_PROFILE_FUNCTION()

	m_EditorCamera.OnUpdate(ts);

	Renderer::BeginScene(m_EditorCamera);

	Renderer::DrawObjects(m_Objects);

	Renderer::EndScene();
}

void SandboxLayer::OnImGuiRender()
{
}

void SandboxLayer::OnEvent(Event& e)
{
	m_EditorCamera.OnEvent(e);

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<FrameBufferResizeEvent>(VKC_BIND_EVENT_FN(SandboxLayer::OnResized));
}

bool SandboxLayer::OnResized(FrameBufferResizeEvent& e)
{
	GraphicsContext::RecreateSwapChain();

	m_EditorCamera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());

	return false;
}