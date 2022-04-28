project "Sandbox"
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
		"%{wks.location}/Hog-Core/vendor/spdlog/include",
		"%{wks.location}/Hog-Core/src",
		"%{wks.location}/Hog-Core/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.tinyobjloader}",
		"%{IncludeDir.cgltf}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.optick}"
	}

	links
	{
		"Hog-Core",
		"%{Library.Vulkan}",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "HG_DEBUG"
		runtime "Debug"
		symbols "on"
		-- editAndContinue "Off"
		
		postbuildcommands
		{
			"{COPY} \"%{SharedLibrary.shaderc_Debug}\" \"%{cfg.targetdir}\""
		}

	filter "configurations:Release"
		defines "HG_RELEASE"
		runtime "Release"
		optimize "on"

		postbuildcommands
		{
			"{COPY} \"%{SharedLibrary.shaderc_Release}\" \"%{cfg.targetdir}\"",
		}

	filter "configurations:Dist"
		defines "HG_DIST"
		runtime "Release"
		optimize "on"
		
		postbuildcommands
		{
			"{COPY} \"%{SharedLibrary.shaderc_Release}\" \"%{cfg.targetdir}\"",
		}

	filter "configurations:Profile"
		defines "HG_PROFILE"
		runtime "Release"
		optimize "on"
		
		postbuildcommands
		{
			"{COPY} \"%{SharedLibrary.optick}\" \"%{cfg.targetdir}\"",
			"{COPY} \"%{SharedLibrary.shaderc_Release}\" \"%{cfg.targetdir}\""
		}
