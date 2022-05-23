#include "GraphicsExample.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Hog/ImGui/ImGuiHelper.h>

static auto& context = GraphicsContext::Get();

GraphicsExample::GraphicsExample()
	: Layer("GraphicsExample")
{

}

void GraphicsExample::OnAttach()
{
	HG_PROFILE_FUNCTION();
	CVarSystem::Get()->SetIntCVar("application.enableImGui", 1);

	GraphicsContext::Initialize();
	
	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.Queue), &(context.QueueFamilyIndex), 1, nullptr);

	Ref<Image> colorAttachment = Renderer::GetFinalRenderTarget();
	Ref<Image> depthAttachment = Image::Create(ImageDescription::Defaults::Depth, 1);

	Ref<Buffer> ViewProjection = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(glm::mat4));
	Ref<Buffer> Materials = Buffer::Create(BufferDescription::Defaults::UniformBuffer, (uint32_t)(sizeof(MaterialGPUData) * MATERIAL_ARRAY_SIZE));
	std::vector<Ref<RendererObject>> rendereObjects;

	RenderGraph graph;
	auto graphics = graph.AddStage(nullptr, {
		"ForwardGraphics", Shader::Create("Basic", "Basic.vertex", "Basic.fragment"), RendererStageType::ForwardGraphics,
		{
			{DataType::Defaults::Float3, "a_Position"},
			{DataType::Defaults::Float3, "a_Normal"},
			{DataType::Defaults::Float2, "a_TexCoords"},
			{DataType::Defaults::Float3, "a_MaterialIndex"},
		},
		{
			{"u_ViewProjection", ResourceType::Uniform, ShaderType::Defaults::Vertex, ViewProjection, 0, 0},
			{"u_Materials", ResourceType::Uniform, ShaderType::Defaults::Fragment, Materials, 1, 0},
			{"u_Textures", ResourceType::SamplerArray, ShaderType::Defaults::Fragment, TextureLibrary::GetLibraryArray(), 2, 0},
			{"p_Model", ResourceType::PushConstant, ShaderType::Defaults::Vertex, sizeof(PushConstant), &m_PushConstant},
		},
		rendereObjects,
		{
			{"Color", AttachmentType::Color, colorAttachment, true},
			{"Depth", AttachmentType::Depth, depthAttachment, true},
		},
	});

	auto imGuiStage = graph.AddStage(graphics, {
		"ImGuiStage", RendererStageType::ImGui, {{"ColorTarget", AttachmentType::Color, colorAttachment}}
	});

	graph.AddStage(imGuiStage, {
		"BlitStage", Shader::Create("Blit", "fullscreen.vertex", "blit.fragment"), RendererStageType::Blit,
		{{"FinalRender", ResourceType::Sampler, ShaderType::Defaults::Fragment, colorAttachment, 0, 0},},
		{{"ColorTarget", AttachmentType::Color, colorAttachment, false, true},},
	});

	Renderer::Initialize(graph);

	for (auto & obj : m_Objects)
	{
		obj->SetTransform(glm::rotate(glm::mat4{ 1.0f }, glm::radians(90.0f), glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4{ 1.0f }, glm::radians(180.0f), glm::vec3(1, 0, 0)));
	}

	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 10000.0f);
}

void GraphicsExample::OnDetach()
{
	HG_PROFILE_FUNCTION()

	GraphicsContext::WaitIdle();

	Renderer::Deinitialize();
	MaterialLibrary::Deinitialize();
	TextureLibrary::Deinitialize();

	m_Objects.clear();

	GraphicsContext::Deinitialize();
}

void GraphicsExample::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION()

	m_EditorCamera.OnUpdate(ts);
}

void GraphicsExample::OnImGuiRender()
{
	ImGui::ShowDemoWindow();
}

void GraphicsExample::OnEvent(Event& e)
{
	m_EditorCamera.OnEvent(e);

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<FrameBufferResizeEvent>(HG_BIND_EVENT_FN(GraphicsExample::OnResized));
}

bool GraphicsExample::OnResized(FrameBufferResizeEvent& e)
{
	GraphicsContext::RecreateSwapChain();

	m_EditorCamera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());

	return false;
}