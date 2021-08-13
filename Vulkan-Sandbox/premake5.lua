project "Vulkan-Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Vulkan-Core/vendor/spdlog/include",
		"%{wks.location}/Vulkan-Core/src",
		"%{wks.location}/Vulkan-Core/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.VulkanSDK}"
	}

	links
	{
		"Vulkan-Core",
		"%{Library.Vulkan}",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "VKC_DEBUG"
		runtime "Debug"
		symbols "on"
		
		postbuildcommands
		{
			"{COPYDIR} \"%{LibraryDir.VulkanSDK_DebugDLL}\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Release"
		defines "VKC_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "VKC_DIST"
		runtime "Release"
		optimize "on"
