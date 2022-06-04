#include "GraphicsExample.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Hog/ImGui/ImGuiHelper.h>

static auto& context = GraphicsContext::Get();

GraphicsExample::GraphicsExample()
	: Layer("GraphicsExample")
{

}

void GraphicsExample::OnAttach()
{
	HG_PROFILE_FUNCTION();
	CVarSystem::Get()->SetIntCVar("application.enableImGui", 0);
	CVarSystem::Get()->SetIntCVar("renderer.enableMipMapping", 1);
	CVarSystem::Get()->SetStringCVar("shader.compilation.macros", "MATERIAL_ARRAY_SIZE=128;TEXTURE_ARRAY_SIZE=512");
	CVarSystem::Get()->SetIntCVar("material.array.size", 128);

	GraphicsContext::Initialize();

	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.Queue), &(context.QueueFamilyIndex), 1, nullptr);

	Ref<Buffer> m_MaterialBuffer { nullptr };

	// LoadGltfFile("assets/models/sponza-intel/NewSponza_Main_Blender_glTF.gltf", m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer);
	LoadGltfFile("assets/models/sponza/sponza.gltf", m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer);
	// LoadGltfFile("assets/models/cube/cube.gltf", m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer);

	Ref<Image> colorAttachment = Image::Create(ImageDescription::Defaults::SampledColorAttachment, 1);
	Ref<Image> depthAttachment = Image::Create(ImageDescription::Defaults::Depth, 1);
	Ref<Texture> colorAttachmentTexture = Texture::Create({}, colorAttachment);

	m_ViewProjection = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(glm::mat4));

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
			{"u_ViewProjection", ResourceType::Uniform, ShaderType::Defaults::Vertex, m_ViewProjection, 0, 0},
			{"u_Materials", ResourceType::Uniform, ShaderType::Defaults::Fragment, MaterialLibrary::GetBuffer(), 1, 0},
			{"u_Textures", ResourceType::SamplerArray, ShaderType::Defaults::Fragment, m_Textures, 2, 0, 512},
			{"p_Model", ResourceType::PushConstant, ShaderType::Defaults::Vertex, sizeof(PushConstant), &m_PushConstant},
		},
		m_OpaqueMeshes,
		{
			{"Color", AttachmentType::Color, colorAttachment, true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::ColorAttachmentOptimal}},
			{"Depth", AttachmentType::Depth, depthAttachment, true, {ImageLayout::DepthStencilAttachmentOptimal, ImageLayout::DepthStencilAttachmentOptimal}},
		},
	});

	auto transparentGraphics = graph.AddStage(graphics, {
		"ForwardGraphics", Shader::Create("Basic", "Basic.vertex", "Basic.fragment"), RendererStageType::ForwardGraphics,
		{
			{DataType::Defaults::Float3, "a_Position"},
			{DataType::Defaults::Float3, "a_Normal"},
			{DataType::Defaults::Float2, "a_TexCoords"},
			{DataType::Defaults::Float3, "a_MaterialIndex"},
		},
		{
			{"u_ViewProjection", ResourceType::Uniform, ShaderType::Defaults::Vertex, m_ViewProjection, 0, 0},
			{"u_Materials", ResourceType::Uniform, ShaderType::Defaults::Fragment, MaterialLibrary::GetBuffer(), 1, 0},
			{"u_Textures", ResourceType::SamplerArray, ShaderType::Defaults::Fragment, m_Textures, 2, 0, 512},
			{"p_Model", ResourceType::PushConstant, ShaderType::Defaults::Vertex, sizeof(PushConstant), &m_PushConstant},
		},
		m_TransparentMeshes,
		{
			{"Color", AttachmentType::Color, colorAttachment, false, {ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal}},
			{"Depth", AttachmentType::Depth, depthAttachment, true, {ImageLayout::DepthStencilAttachmentOptimal, ImageLayout::DepthStencilAttachmentOptimal}},
		},
	});

	//auto imGuiStage = graph.AddStage(graphics, {
	//	"ImGuiStage", RendererStageType::ImGui, {
	//		{"ColorTarget", AttachmentType::Color, colorAttachment, false, {
	//			PipelineStage::ColorAttachmentOutput, AccessFlag::ColorAttachmentWrite,
	//			PipelineStage::ColorAttachmentOutput, AccessFlag::ColorAttachmentRead,
	//			ImageLayout::ColorAttachmentOptimal,
	//			ImageLayout::ShaderReadOnlyOptimal
	//		}},
	//	}
	//});

	graph.AddStage(transparentGraphics, {
		"BlitStage", Shader::Create("Blit", "fullscreen.vertex", "blit.fragment", false), RendererStageType::Blit,
		{{"FinalRender", ResourceType::Sampler, ShaderType::Defaults::Fragment, colorAttachmentTexture, 0, 0, {
				PipelineStage::ColorAttachmentOutput, AccessFlag::ColorAttachmentWrite,
				PipelineStage::FragmentShader, AccessFlag::ShaderSampledRead,
		}},},
		{{"SwapchainImage", AttachmentType::Swapchain, true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::PresentSrcKHR}},},
	});

	Renderer::Initialize(graph);

	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 10000.0f);
}

void GraphicsExample::OnDetach()
{
	HG_PROFILE_FUNCTION()

	GraphicsContext::WaitIdle();

	Renderer::Cleanup();
	MaterialLibrary::Clneaup();

	m_OpaqueMeshes.clear();
	m_TransparentMeshes.clear();
	m_Textures.clear();
	m_ViewProjection.reset();

	GraphicsContext::Deinitialize();
}

void GraphicsExample::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION();

	m_EditorCamera.OnUpdate(ts);
	glm::mat4 viewProj = m_Cameras.begin()->second;
	m_ViewProjection->WriteData(&viewProj, sizeof(viewProj));
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