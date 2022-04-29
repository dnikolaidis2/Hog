include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Hog"
	architecture "x86_64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist",
		"Profile",
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
	include "Hog-Core/vendor/GLFW"
	include "Hog-Core/vendor/imgui"
	include "Hog-Core/vendor/yaml-cpp"
	include "Hog-Core/vendor/optick/premake5.core.lua"
group ""

include "Hog-Core"
include "Sandbox"
