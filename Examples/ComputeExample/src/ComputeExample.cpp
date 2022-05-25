#include "ComputeExample.h"

static auto& context = GraphicsContext::Get();
constexpr uint32_t BufferElements = 32;

ComputeExample::ComputeExample()
	: Layer("ComputeExample")
{

}

void ComputeExample::OnAttach()
{
	HG_PROFILE_FUNCTION();

	CVarSystem::Get()->SetIntCVar("application.enableImGui", 0);

	GraphicsContext::Initialize();

	HG_PROFILE_GPU_INIT_VULKAN(&(context.Device), &(context.PhysicalDevice), &(context.Queue), &(context.QueueFamilyIndex), 1, nullptr);

	m_ComputeBuffer = Buffer::Create(BufferDescription::Defaults::ReadbackStorageBuffer, BufferElements * sizeof(uint32_t));

	uint32_t n = 0;
	std::vector<uint32_t> tempBuffer(BufferElements);
	std::generate(tempBuffer.begin(), tempBuffer.end(), [&n] { return n++; });
	
	m_ComputeBuffer->WriteData(tempBuffer.data(), tempBuffer.size() * sizeof(uint32_t));

	RenderGraph graph;
	uint32_t bufferElements = 32;

	auto fib = graph.AddStage(nullptr, { "Fibonacci stage", Shader::Create("Headless.compute"), RendererStageType::ForwardCompute,
		{
			{"values", ResourceType::Storage, ShaderType::Defaults::Compute, m_ComputeBuffer, 0, 0},
			{ "BUFFER_ELEMENTS", ResourceType::Constant, ShaderType::Defaults::Compute, 0, sizeof(uint32_t), &bufferElements}
		},
		{bufferElements, 1, 1}
	});

	Renderer::Initialize(graph);

	std::vector<uint32_t> computeBuffer(BufferElements);
	m_ComputeBuffer->ReadData(computeBuffer.data(), BufferElements);

	HG_INFO("Before fibonacci stage");
	for (int i = 0; i < computeBuffer.size(); i++)
	{
		HG_TRACE("computeBuffer[{0}] = {1}", i, computeBuffer[i]);
	}
}

void ComputeExample::OnDetach()
{
	HG_PROFILE_FUNCTION()

	std::vector<uint32_t> computeBuffer(BufferElements);
	HG_INFO("After fibonacci stage");
	m_ComputeBuffer->ReadData(computeBuffer.data(), BufferElements);
	for (int i = 0; i < computeBuffer.size(); i++)
	{
		HG_TRACE("computeBuffer[{0}] = {1}", i, computeBuffer[i]);
	}

	GraphicsContext::WaitIdle();

	Renderer::Cleanup();

	m_ComputeBuffer.reset();

	GraphicsContext::Deinitialize();
}

void ComputeExample::OnUpdate(Timestep ts)
{
	HG_PROFILE_FUNCTION()

	Application::Get().Close();
}

void ComputeExample::OnImGuiRender()
{
}