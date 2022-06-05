
#include <Hog.h>
#include <Hog/Core/EntryPoint.h>

#include "DeferredExample.h"

class Sandbox : public Hog::Application
{
public:
	Sandbox(Hog::ApplicationCommandLineArgs args)
		: Application("Deferred Example", args)
	{
		PushLayer(CreateRef<DeferredExample>());
	}

	~Sandbox()
	{
	}
};

Hog::Application* Hog::CreateApplication(Hog::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
