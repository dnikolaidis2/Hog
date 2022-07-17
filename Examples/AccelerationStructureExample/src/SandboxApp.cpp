
#include <Hog.h>
#include <Hog/Core/EntryPoint.h>

#include "AccelerationStructureExample.h"

class Sandbox : public Hog::Application
{
public:
	Sandbox(Hog::ApplicationCommandLineArgs args)
		: Application("Compute Example", args)
	{
		PushLayer(CreateRef<AccelerationStructureExample>());
	}

	~Sandbox()
	{
	}
};

Hog::Application* Hog::CreateApplication(Hog::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
