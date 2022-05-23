
#include <Hog.h>
#include <Hog/Core/EntryPoint.h>

#include "ComputeExample.h"

class Sandbox : public Hog::Application
{
public:
	Sandbox(Hog::ApplicationCommandLineArgs args)
		: Application("Compute Example", args)
	{
		PushLayer(CreateRef<ComputeExample>());
	}

	~Sandbox()
	{
	}
};

Hog::Application* Hog::CreateApplication(Hog::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
