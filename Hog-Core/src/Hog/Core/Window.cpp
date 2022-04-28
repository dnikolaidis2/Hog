#include "hgpch.h"
#include "Hog/Core/Window.h"

#ifdef HG_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Hog
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
	#ifdef HG_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
	#else
		HG_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}

}