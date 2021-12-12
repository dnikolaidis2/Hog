#pragma once
#include "VulkanCore/Core/Base.h"
#include "VulkanCore/Core/Application.h"

#ifdef VKC_PLATFORM_WINDOWS

extern VulkanCore::Application* VulkanCore::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	VulkanCore::Log::Init();

	VKC_PROFILE_BEGIN_SESSION("Startup", "HazelProfile-Startup.json");
	auto app = VulkanCore::CreateApplication({ argc, argv });
	VKC_PROFILE_END_SESSION();

	VKC_PROFILE_BEGIN_SESSION("Runtime", "HazelProfile-Runtime.json");
	app->Run();
	VKC_PROFILE_END_SESSION();

	VKC_PROFILE_BEGIN_SESSION("Shutdown", "HazelProfile-Shutdown.json");
	delete app;
	VKC_PROFILE_END_SESSION();
}

#endif
