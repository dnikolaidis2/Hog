#include "vkcpch.h"
#include "VulkanCore/Core/Window.h"

#ifdef VKC_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace VulkanCore
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
	#ifdef VKC_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
	#else
		VKC_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}

}