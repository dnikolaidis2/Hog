#include "SandboxLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>

static auto& context = GraphicsContext::Get();

SandboxLayer::SandboxLayer()
	: Layer("SandboxLayer")
{

}

void SandboxLayer::OnAttach()
{
	HG_PROFILE_FUNCTION()

#if HG_PROFILE
	context.EnableValidationLayers = false;
#endif

	GraphicsContext::Initialize();

	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.GraphicsQueue), &(context.GraphicsFamilyIndex), 1, nullptr);

	ShaderLibrary::LoadDirectory("assets/shaders");
	Renderer::Initialize();
	TextureLibrary::Initialize();

	m_ImGuiLayer = CreateRef<ImGuiLayer>();
	Application::Get().SetImGuiLayer(m_ImGuiLayer);

	LoadObjFile("assets/models/sponza/sponza.obj", m_Objects);
	// LoadObjFile("assets/models/monkey/monkey_flat.obj", m_Objects);

	for (auto & obj : m_Objects)
	{
		obj->SetTransform(glm::rotate(glm::mat4{ 1.0f }, glm::radians(90.0f), glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4{ 1.0f }, glm::radians(180.0f), glm::vec3(1, 0, 0)));
	}

	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 10000.0f);
}

void SandboxLayer::OnDetach()
{
	HG_PROFILE_FUNCTION()

	GraphicsContext::WaitIdle();

	Application::Get().PopOverlay(m_ImGuiLayer);
	Renderer::Deinitialize();
	ShaderLibrary::Deinitialize();
	MaterialLibrary::Deinitialize();
	TextureLibrary::Deinitialize();

	m_Objects.clear();

	GraphicsContext::Deinitialize();
}

void SandboxLayer::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION()

	m_EditorCamera.OnUpdate(ts);

	Renderer::GlobalShaderData data;
	Renderer::BeginScene(m_EditorCamera, data);

	Renderer::DrawObjects(m_Objects);
}

void SandboxLayer::OnImGuiRender()
{
	ImGui::ShowDemoWindow();
}

void SandboxLayer::OnEvent(Event& e)
{
	m_EditorCamera.OnEvent(e);

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<FrameBufferResizeEvent>(HG_BIND_EVENT_FN(SandboxLayer::OnResized));
}

bool SandboxLayer::OnResized(FrameBufferResizeEvent& e)
{
	GraphicsContext::RecreateSwapChain();

	m_EditorCamera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());

	return false;
}