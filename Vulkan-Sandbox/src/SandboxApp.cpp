
#include <VulkanCore.h>
#include <VulkanCore/Core/EntryPoint.h>

#include "SandboxLayer.h"

class Sandbox : public VulkanCore::Application
{
public:
	Sandbox(VulkanCore::ApplicationCommandLineArgs args)
		: Application("Vulkan Sandbox", args)
	{
		PushLayer(new SandboxLayer());
	}

	~Sandbox()
	{
	}
};

VulkanCore::Application* VulkanCore::CreateApplication(VulkanCore::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
