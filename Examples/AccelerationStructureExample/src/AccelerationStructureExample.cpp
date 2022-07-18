#include "AccelerationStructureExample.h"

static auto& context = GraphicsContext::Get();
constexpr uint32_t BufferElements = 32;

AccelerationStructureExample::AccelerationStructureExample()
	: Layer("AccelerationStructureExample")
{

}

void AccelerationStructureExample::OnAttach()
{
	HG_PROFILE_FUNCTION();

	CVarSystem::Get()->SetIntCVar("application.enableImGui", 0);

	ShaderCache::Initialize();
	GraphicsContext::Initialize();

	LoadGltfFile("assets/models/sponza/sponza.gltf", {}, m_OpaqueMeshes, m_TransparentMeshes, m_Cameras, m_Textures, m_Materials, m_MaterialBuffer, m_Lights, m_LightBuffer);

	m_TopLevelAS = AccelerationStructure::Create(m_OpaqueMeshes);

	m_ViewProjection = Buffer::Create(BufferDescription::Defaults::UniformBuffer, 2 * sizeof(glm::mat4));

	auto storageImage = Image::Create(ImageDescription::Defaults::Storage, 1, VK_FORMAT_B8G8R8A8_UNORM);

	RenderGraph graph;

	auto raytrace = graph.AddStage(nullptr, 
		{ "Fibonacci stage", RendererStageType::RayTracing, 
		RayTracingPipeline::Create(
			{
				.Shaders = {
					"raygen.raygen",
					"miss.miss",
					"closesthit.closesthit",
				},
				.MaxRayRecursionDepth = 1,
			}
		),
		{
			{"TLAS", ResourceType::AccelerationStructure, ShaderType::Defaults::RayGeneration, m_TopLevelAS, 0, 0},
			{"storage", ResourceType::StorageImage, ShaderType::Defaults::RayGeneration, storageImage, 0, 1, {
				ImageLayout::Undefined, ImageLayout::General,
			}},
			{"storage", ResourceType::Uniform, ShaderType::Defaults::RayGeneration, m_ViewProjection, 0, 2},
		},
		{storageImage->GetWidth(), storageImage->GetHeight(), 1}
	});

	graph.AddStage(raytrace, {
		"BlitStage", RendererStageType::Blit,
		GraphicsPipeline::Create({
			.Shaders = {"fullscreen.vertex", "blit.fragment"},
			.Rasterizer = {
				.CullMode = CullMode::Front,
			},
			.BlendAttachments = {
				{
					.Enable = false,
				},
			},
		}),
		{
			{"FinalRender", ResourceType::Sampler, ShaderType::Defaults::Fragment, Texture::Create(storageImage), 0, 0, {
				ImageLayout::General, ImageLayout::ColorAttachmentOptimal,
			}},
		},
		{{"SwapchainImage", AttachmentType::Swapchain, true, {ImageLayout::ColorAttachmentOptimal, ImageLayout::PresentSrcKHR}},},
		});

	Renderer::Initialize(graph);
}

void AccelerationStructureExample::OnDetach()
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
	m_TopLevelAS.reset();

	GraphicsContext::Deinitialize();
}

void AccelerationStructureExample::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION()

	Application::Get().Close();
}

void AccelerationStructureExample::OnImGuiRender()
{
}