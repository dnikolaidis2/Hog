
-- Hog-Core Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

SourceDir = {}
SourceDir["optick"] = "%{wks.location}/Hog-Core/vendor/optick/src"

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/Hog-Core/vendor/stb_image"
IncludeDir["cgltf"] = "%{wks.location}/Hog-Core/vendor/cgltf"
IncludeDir["vma"] = "%{wks.location}/Hog-Core/vendor/vma"
IncludeDir["yaml_cpp"] = "%{wks.location}/Hog-Core/vendor/yaml-cpp/include"
IncludeDir["tinyobjloader"] = "%{wks.location}/Hog-Core/vendor/tinyobjloader"
IncludeDir["GLFW"] = "%{wks.location}/Hog-Core/vendor/GLFW/include"
IncludeDir["ImGui"] = "%{wks.location}/Hog-Core/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/Hog-Core/vendor/glm"
IncludeDir["shaderc"] = "%{wks.location}/Hog-Core/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Hog-Core/vendor/SPIRV-Cross"
IncludeDir["SPIRV_Reflect"] = "%{wks.location}/Hog-Core/vendor/spirv-reflect"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["optick"] = "%{wks.location}/Hog-Core/vendor/optick/src"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_DebugDLL"] = "%{VULKAN_SDK}/Bin"
LibraryDir["optick"] = "%{wks.location}/Hog-Core/vendor/optick"

Library = {}
Library["optick"] = "%{LibraryDir.optick}/OptickCore.lib"

Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

SharedLibrary = {}
SharedLibrary["optick"] = "%{LibraryDir.optick}/OptickCore.dll"
SharedLibrary["shaderc_Release"] = "%{LibraryDir.VulkanSDK_DebugDLL}/shaderc_shared.dll"
SharedLibrary["shaderc_Debug"] = "%{LibraryDir.VulkanSDK_DebugDLL}/shaderc_sharedd.dll"