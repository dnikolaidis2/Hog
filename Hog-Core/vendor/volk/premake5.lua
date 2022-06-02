project "Volk"
	kind "StaticLib"
	language "C"
	cdialect "C89"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"volk.h",
		"volk.c"
	}

	includedirs
	{
		"%{IncludeDir.volk}",
		"%{IncludeDir.VulkanSDK}",
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"VK_USE_PLATFORM_WIN32_KHR",
		}

	filter "configurations:Asan"
		defines "HG_ASAN"
		defines "HG_DEBUG"
		runtime "Debug"
		symbols "on"
		editAndContinue "Off"
		flags { "NoRuntimeChecks" }
		buildoptions { "/Zi /DEBUG:FULL /Ob0 /Oy-" }

	filter "configurations:Debug"
		defines "HG_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "HG_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "HG_DIST"
		runtime "Release"
		optimize "on"

	filter "configurations:Profile"
		defines "HG_PROFILE"
		runtime "Release"
		optimize "on"
