include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Vulkan-Sandbox"
	architecture "x86_64"
	startproject "Vulkan-Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "Vulkan-Core/vendor/GLFW"
	include "Vulkan-Core/vendor/imgui"
	include "Vulkan-Core/vendor/yaml-cpp"
group ""

include "Vulkan-Core"
include "Vulkan-Sandbox"
