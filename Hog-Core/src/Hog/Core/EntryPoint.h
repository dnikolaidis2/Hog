#pragma once
#include "Hog/Core/Base.h"
#include "Hog/Core/Application.h"

#ifdef HG_PLATFORM_WINDOWS

extern Hog::Application* Hog::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	Hog::Log::Init();

	HG_PROFILE_BEGIN_SESSION("Startup", "HogProfile-Startup.json");
	auto app = Hog::CreateApplication({ argc, argv });
	HG_PROFILE_END_SESSION();

	HG_PROFILE_BEGIN_SESSION("Runtime", "HogProfile-Runtime.json");
	app->Run();
	HG_PROFILE_END_SESSION();

	HG_PROFILE_BEGIN_SESSION("Shutdown", "HogProfile-Shutdown.json");
	delete app;
	HG_PROFILE_END_SESSION();
}

#endif
