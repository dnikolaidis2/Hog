
-- Vulkan-Core Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

SourceDir = {}
SourceDir["optick"] = "%{wks.location}/vendor/optick/src"

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/Vulkan-Core/vendor/stb_image"
IncludeDir["vma"] = "%{wks.location}/Vulkan-Core/vendor/vma"
IncludeDir["tinyobjloader"] = "%{wks.location}/Vulkan-Core/vendor/tinyobjloader"
IncludeDir["GLFW"] = "%{wks.location}/Vulkan-Core/vendor/GLFW/include"
IncludeDir["ImGui"] = "%{wks.location}/Vulkan-Core/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/Vulkan-Core/vendor/glm"
IncludeDir["shaderc"] = "%{wks.location}/Vulkan-Core/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Vulkan-Core/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["optick"] = "%{wks.location}/vendor/optick/include"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{wks.location}/Vulkan-Core/vendor/VulkanSDK/Lib"
LibraryDir["VulkanSDK_DebugDLL"] = "%{wks.location}/Vulkan-Core/vendor/VulkanSDK/Bin"
LibraryDir["optick"] = "%{wks.location}/vendor/optick/lib/x64/release"
LibraryDir["optick_Debug"] = "%{wks.location}/vendor/optick/lib/x64/debug"

Library = {}
Library["optick_Release"] = "%{LibraryDir.optick}/OptickCore.lib"
Library["optick_ReleaseDLL"] = "%{LibraryDir.optick}/OptickCore.dll"
Library["optick_Debug"] = "%{LibraryDir.optick_Debug}/OptickCore.lib"
Library["optick_DebugDLL"] = "%{LibraryDir.optick_Debug}/OptickCore.dll"

Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"
