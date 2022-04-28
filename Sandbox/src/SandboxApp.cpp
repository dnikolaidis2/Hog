
#include <Hog.h>
#include <Hog/Core/EntryPoint.h>

#include "SandboxLayer.h"

class Sandbox : public Hog::Application
{
public:
	Sandbox(Hog::ApplicationCommandLineArgs args)
		: Application("Vulkan Sandbox", args)
	{
		PushLayer(CreateRef<SandboxLayer>());
	}

	~Sandbox()
	{
	}
};

Hog::Application* Hog::CreateApplication(Hog::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
