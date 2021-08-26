#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCore/Core/Base.h"

namespace VulkanCore {

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch(messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			{
				VKC_CORE_INFO(pCallbackData->pMessage);
			} break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			{
				VKC_CORE_WARN(pCallbackData->pMessage);
			} break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			{
				VKC_CORE_ERROR(pCallbackData->pMessage);
			} break;
		}

		return VK_FALSE;
	}

	class GraphicsContext
	{
	public:
		struct GPUInfo
		{
			VkPhysicalDevice Device;
			std::vector<VkQueueFamilyProperties> QueueFamilyProperties;
			std::vector<VkExtensionProperties> ExtensionProperties;
			VkSurfaceCapabilitiesKHR SurfaceCapabilities;
			std::vector<VkSurfaceFormatKHR> SurfaceFormats;
			std::vector<VkPresentModeKHR> PresentModes;
			VkPhysicalDeviceMemoryProperties MemoryProperties;
			VkPhysicalDeviceProperties DeviceProperties;
		};

		struct ContextData
		{
			VkApplicationInfo ApplicationInfo = {
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Test app",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "Hazel",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION)
			};

			VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
				.pfnUserCallback = DebugCallback,
				.pUserData = nullptr // Optional
			};

			std::vector<const char*> InstanceExtensions;
			std::vector<const char*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			bool EnableValidationLayers = true;
			std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

			std::vector<GPUInfo> GPUs;

			VkInstance Instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;


			GPUInfo* GPU;
			VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

			uint32_t GraphicsFamilyIndex;
			uint32_t PresentFamilyIndex;

			VkSurfaceKHR Surface = VK_NULL_HANDLE;
		};

	public:
		static GraphicsContext& Get()
		{
			static GraphicsContext instance;

			return instance;
		}

		static void Initialize() { Get().InitializeImpl(); }
		static void Deinitialize() { Get().DeinitializeImpl(); }
		static ContextData& GetContext() { return Get().m_Context; }
		static VkInstance& GetInstance() { return Get().m_Context.Instance; }
	public:
		GraphicsContext(GraphicsContext const&) = delete;
		void operator=(GraphicsContext const&) = delete;
	private:
		GraphicsContext() = default;

		void InitializeImpl();
		void DeinitializeImpl();

		void CreateInstance();
		void DestroyInstance();
		void DestroySurface();
		void SetupDebugMessenger();
		void EnumeratePhysicalDevices();
		void SelectPhysicalDevice();
		
	private:
		ContextData m_Context;
	};
}