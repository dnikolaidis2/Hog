project "Hog-Core"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "hgpch.h"
	pchsource "src/hgpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/stb_image_write/**.h",
		"vendor/stb_image_write/**.cpp",
		"vendor/json/**.hpp",
		"vendor/vma/**.h",
		"vendor/vma/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
		"vendor/spirv-reflect/spirv_reflect.cpp",
		"vendor/tinygltf/**.h",
		"vendor/tinygltf/**.cpp",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"VK_VERSION_1_3",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_ENABLE_EXPERIMENTAL",
		"VMA_STATIC_VULKAN_FUNCTIONS",
		"VMA_DYNAMIC_VULKAN_FUNCTIONS=0",
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.stb_image_write}",
		"%{IncludeDir.json}",
		"%{IncludeDir.tinygltf}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.optick}",
		"%{IncludeDir.SPIRV_Reflect}",
		"%{IncludeDir.volk}",
		"%{IncludeDir.VulkanSDK}",
	}

	links
	{
		"GLFW",
		"yaml-cpp",
		"ImGui",
		"volk"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "HG_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}

	filter "configurations:Asan"
		defines "HG_ASAN"
		defines "HG_DEBUG"
		runtime "Debug"
		symbols "on"
		editAndContinue "Off"
		flags { "NoRuntimeChecks" }
		buildoptions { "/Zi /DEBUG:FULL /Ob0 /Oy-" }
		
		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}

	filter "configurations:Release"
		defines "HG_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Dist"
		defines "HG_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Profile"
		defines "HG_PROFILE"
		runtime "Release"
		optimize "on"

		links
		{
			"OptickCore",
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}
