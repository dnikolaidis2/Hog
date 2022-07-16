#include "DeferredExample.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Hog/ImGui/ImGuiHelper.h>

static auto& context = GraphicsContext::Get();

DeferredExample::DeferredExample()
	: Layer("DeferredExample")
{

}

void DeferredExample::OnAttach()
{
	HG_PROFILE_FUNCTION();
	CVarSystem::Get()->SetIntCVar("application.enableImGui", 0);
	CVarSystem::Get()->SetIntCVar("renderer.enableMipMapping", 1);
	CVarSystem::Get()->SetStringCVar("shader.compilation.macros", "MATERIAL_ARRAY_SIZE=128;TEXTURE_ARRAY_SIZE=512;LIGHT_ARRAY_SIZE=32");

	GraphicsContext::Initialize();

	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.Queue), &(context.QueueFamilyIndex), 1, nullptr);

	// LoadGltfFile("assets/models/sponza-intel/NewSponza_Main_Blender_glTF.gltf", {}, m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer, m_Lights, m_LightBuffer);
	LoadGltfFile("assets/models/sponza/sponza.gltf", {}, m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer, m_Lights, m_LightBuffer);
	// LoadGltfFile("assets/models/armor/armor-test.gltf", {}, m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer, m_Lights, m_LightBuffer);
	// LoadGltfFile("assets/models/cube/cube.gltf", {}, m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer, m_Lights, m_LightBuffer);
	// LoadGltfFile("assets/models/plane/plane.gltf", {}, m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer, m_Lights, m_LightBuffer);

	Ref<Texture> shadowMap = Texture::Create(Image::Create(ImageDescription::Defaults::ShadowMap, 4096, 4096, 1, static_cast<VkFormat>(DataType::Defaults::Depth32)));

	Ref<Texture> albedoAttachment = Texture::Create(Image::Create(ImageDescription::Defaults::SampledColorAttachment, 1));
	Ref<Texture> positionAttachment = Texture::Create(Image::Create(ImageDescription::Defaults::SampledPositionAttachment, 1));
	Ref<Texture> normalAttachment = Texture::Create(Image::Create(ImageDescription::Defaults::SampledNormalAttachment, 1));
	Ref<Texture> depthAttachment = Texture::Create(Image::Create(ImageDescription::Defaults::Depth, 1));

	Ref<Texture> colorAttachment = Texture::Create(Image::Create(ImageDescription::Defaults::SampledHDRColorAttachment, 1));

	m_ViewProjection = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(glm::mat4));
	m_LightViewProjection = Buffer::Create(BufferDescription::Defaults::UniformBuffer, sizeof(glm::mat4));
	uint32_t lightCount = m_Lights.size();

	RenderGraph graph;

	auto shadowPass = graph.AddStage(nullptr, {
		"Shadow Pass", RendererStageType::ForwardGraphics, GraphicsPipeline::Create({
				.Shaders = {"Shadow.vertex", "Shadow.fragment"},
			}
		),
		{
			{DataType::Defaults::Float3, "a_Position"},
		},
		{
			{"u_ViewProjection", ResourceType::Uniform, ShaderType::Defaults::Vertex, m_LightViewProjection, 0, 0},
			{"p_Model", ResourceType::PushConstant, ShaderType::Defaults::Vertex, sizeof(PushConstant), &m_PushConstant},
		},
		m_OpaqueMeshes,
		{
			{"Shadow Map", AttachmentType::Depth, shadowMap->GetImage(), true, {ImageLayout::DepthStencilAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal}},
		},
	});

	auto gbuffer = graph.AddStage(shadowPass, {
		"GBuffer", RendererStageType::ForwardGraphics, GraphicsPipeline::Create({
				.Shaders = {"GBuffer.vertex", "GBuffer.fragment"},
				.BlendAttachments = {{}, {}, {},},
			}
		),
		{
			{DataType::Defaults::Float3, "a_Position"},
			{DataType::Defaults::Float2, "a_TexCoords"},
			{DataType::Defaults::Float3, "a_Normal"},
			{DataType::Defaults::Float4, "a_Tangent"},
			{DataType::Defaults::Int, "a_MaterialIndex"},
		},
		{
			{"u_ViewProjection", ResourceType::Uniform, ShaderType::Defaults::Vertex, m_ViewProjection, 0, 0},
			{"u_Materials", ResourceType::Uniform, ShaderType::Defaults::Fragment, m_MaterialBuffer, 0, 1},
			{"u_Textures", ResourceType::SamplerArray, ShaderType::Defaults::Fragment, m_Textures, 0, 2, 512},
			{"p_Model", ResourceType::PushConstant, ShaderType::Defaults::Vertex, sizeof(PushConstant), &m_PushConstant},
		},
		m_OpaqueMeshes,
		{
			{"Position", AttachmentType::Color, positionAttachment->GetImage(), true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal}},
			{"Normal", AttachmentType::Color, normalAttachment->GetImage(), true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal}},
			{"Albedo", AttachmentType::Color, albedoAttachment->GetImage(), true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal}},
			{"Depth", AttachmentType::Depth, depthAttachment->GetImage(), true, {ImageLayout::DepthStencilAttachmentOptimal, ImageLayout::DepthStencilAttachmentOptimal}},
		},
	});

	auto lightingPass = graph.AddStage(gbuffer, {
		"Lighting stage", RendererStageType::ScreenSpacePass, GraphicsPipeline::Create({
				.Shaders = {"fullscreen.vertex", "Lighting.fragment"},
				.Rasterizer = {
					.CullMode = CullMode::Front,
				},
			}
		),
		{
			{"u_Position", ResourceType::Sampler, ShaderType::Defaults::Fragment, positionAttachment, 0, 0},
			{"u_Normal", ResourceType::Sampler, ShaderType::Defaults::Fragment, normalAttachment, 0, 1},
			{"u_Albedo", ResourceType::Sampler, ShaderType::Defaults::Fragment, albedoAttachment, 0, 2},
			{"u_Lights", ResourceType::Uniform, ShaderType::Defaults::Fragment, m_LightBuffer, 0, 3},
			{"c_LightCount", ResourceType::Constant, ShaderType::Defaults::Fragment, 0, sizeof(uint32_t), &lightCount},
		},
		{
			{"Color", AttachmentType::Color, colorAttachment->GetImage(), true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal}},
		},
	});

	graph.AddStage(lightingPass, {
		"BlitStage", RendererStageType::Blit, GraphicsPipeline::Create({
				.Shaders = {"fullscreen.vertex", "ToneMapping.fragment"},
				.Rasterizer = {
					.CullMode = CullMode::Front,
				},
				.BlendAttachments = {	
					{
						.Enable = false
					}
				}
			}
		),
		{{"FinalRender", ResourceType::Sampler, ShaderType::Defaults::Fragment, colorAttachment, 0, 0, {
				PipelineStage::ColorAttachmentOutput, AccessFlag::ColorAttachmentWrite,
				PipelineStage::FragmentShader, AccessFlag::ShaderSampledRead,
		}},},
		{{"SwapchainImage", AttachmentType::Swapchain, true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::PresentSrcKHR}},},
	});

	Renderer::Initialize(graph);

	m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 10000.0f);
}

void DeferredExample::OnDetach()
{
	HG_PROFILE_FUNCTION()

	GraphicsContext::WaitIdle();

	Renderer::Cleanup();

	m_OpaqueMeshes.clear();
	m_TransparentMeshes.clear();
	m_Textures.clear();
	m_Materials.clear();
	m_Lights.clear();
	m_MaterialBuffer.reset();
	m_LightBuffer.reset();
	m_ViewProjection.reset();
	m_LightViewProjection.reset();

	GraphicsContext::Deinitialize();
}

void DeferredExample::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION();

	float nearPlane = 1.0f, farPlane = 70.0f;
	glm::mat4 lightViewProj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, nearPlane, farPlane)
		 * glm::lookAt(m_Lights[0]->GetLightData().Position,
						glm::vec3(0.0f), 
						glm::vec3(0.0f, 0.0f, 1.0f));
	m_LightViewProjection->WriteData(&lightViewProj, sizeof(lightViewProj));

	m_EditorCamera.OnUpdate(ts);
	// glm::mat4 viewProj = m_EditorCamera.GetViewProjection();
	glm::mat4 viewProj = m_Cameras.begin()->second;
	m_ViewProjection->WriteData(&viewProj, sizeof(viewProj));
}

void DeferredExample::OnImGuiRender()
{
	ImGui::ShowDemoWindow();
}

void DeferredExample::OnEvent(Event& e)
{
	m_EditorCamera.OnEvent(e);

	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<FrameBufferResizeEvent>(HG_BIND_EVENT_FN(DeferredExample::OnResized));
}

bool DeferredExample::OnResized(FrameBufferResizeEvent& e)
{
	GraphicsContext::RecreateSwapChain();

	m_EditorCamera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());

	return false;
}
