project "Vulkan-Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	defines
	{
		
	}

	includedirs
	{
		"%{wks.location}/Vulkan-Core/vendor/spdlog/include",
		"%{wks.location}/Vulkan-Core/src",
		"%{wks.location}/Vulkan-Core/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.tinyobjloader}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.optick}"
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
		-- editAndContinue "Off"
		
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

	filter "configurations:Profile"
		defines "VKC_PROFILE"
		runtime "Release"
		optimize "on"
		
		postbuildcommands
		{
			"{COPY} \"%{Library.optickDLL}\" \"%{cfg.targetdir}\""
		}
