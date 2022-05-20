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

	GraphicsContext::Initialize();
	
	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.Queue), &(context.QueueFamilyIndex), 1, nullptr);

	RenderGraph graph;
	auto graphics = graph.AddStage(nullptr, {
		"ForwardGraphics", Shader::Create("Basic", "Basic.vertex", "Basic.fragment"), RendererStageType::ForwardGraphics,
		{
			{DataType::Float3, "a_Position"},
			{DataType::Float3, "a_Normal"},
			{DataType::Float2, "a_TexCoords"},
			{DataType::Float3, "a_MaterialIndex"},
		},
		{
			{"u_ViewProjection", ResourceType::Uniform, ResourceBindLocation::Vertex, nullptr, 0, 0},
			{"u_Materials", ResourceType::Uniform, ResourceBindLocation::Fragment, nullptr, 1, 0},
			{"u_Textures", ResourceType::SamplerArray, ResourceBindLocation::Fragment, nullptr, 2, 0},
			{"p_Model", ResourceType::PushConstant, ResourceBindLocation::Vertex, sizeof(PushConstant), &m_PushConstant},
		},
		nullptr,
		nullptr,
	});

	Renderer::Initialize(graph);

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

	Renderer::Deinitialize();
	MaterialLibrary::Deinitialize();
	TextureLibrary::Deinitialize();

	m_Objects.clear();

	GraphicsContext::Deinitialize();
}

void SandboxLayer::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION()

	m_EditorCamera.OnUpdate(ts);
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