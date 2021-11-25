project "Vulkan-Core"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "vkcpch.h"
	pchsource "src/vkcpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/vma/**.h",
		"vendor/vma/**.cpp",
		"vendor/tinyobjloader/**.h",
		"vendor/tinyobjloader/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_VULKAN"
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.tinyobjloader}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.optick}"
	}

	links
	{
		"GLFW",
		"ImGui",
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
		}

	filter "configurations:Debug"
		defines "VKC_DEBUG"
		runtime "Debug"
		symbols "on"
		-- editAndContinue "Off"

		links
		{
			"%{Library.optick_Debug}",
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}

	filter "configurations:Release"
		defines "VKC_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.optick_Release}",
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Dist"
		defines "VKC_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.optick_Release}",
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}
