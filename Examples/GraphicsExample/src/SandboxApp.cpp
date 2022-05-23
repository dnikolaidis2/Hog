
#include <Hog.h>
#include <Hog/Core/EntryPoint.h>

#include "GraphicsExample.h"

class Sandbox : public Hog::Application
{
public:
	Sandbox(Hog::ApplicationCommandLineArgs args)
		: Application("Graphics Example", args)
	{
		PushLayer(CreateRef<GraphicsExample>());
	}

	~Sandbox()
	{
	}
};

Hog::Application* Hog::CreateApplication(Hog::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
