#pragma once

#include <vulkan/vulkan.h>

#include "Image.h"
#include "vk_mem_alloc.h"
#include "Hog/Core/Base.h"

namespace Hog {

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		{
			HG_CORE_TRACE(pCallbackData->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		{
			HG_CORE_WARN(pCallbackData->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		{
			HG_CORE_ERROR(pCallbackData->pMessage);
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
			VkPhysicalDeviceFeatures PhysicalDeviceFeatures;

			VkPhysicalDeviceVulkan13Features PhysicalDeviceVulkan13Features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			};

			VkPhysicalDeviceVulkan12Features PhysicalDeviceVulkan12Features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
				.pNext = &PhysicalDeviceVulkan13Features,
			};

			VkPhysicalDeviceVulkan11Features PhysicalDeviceVulkan11Features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
				.pNext = &PhysicalDeviceVulkan12Features,
			};

			VkPhysicalDeviceFeatures2  PhysicalDeviceFeatures2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
				.pNext = &PhysicalDeviceVulkan11Features,
			};
		};

	public:
		static GraphicsContext& Get()
		{
			static GraphicsContext instance;

			return instance;
		}

		~GraphicsContext() { if (m_Initialized == true) Get().Deinitialize(); }

		static void Initialize() { Get().InitializeImpl(); }
		static void Deinitialize() { Get().DeinitializeImpl(); }
		static void RecreateSwapChain() { Get().RecreateSwapChainImpl(); }
		static void WaitIdle() { Get().WaitIdleImpl(); }
		static VkDescriptorPool GetImGuiDescriptorPool() { Get().GetImGuiDescriptorPoolImpl(); return Get().ImGuiDescriptorPool; }
		static void DestroyImGuiDescriptorPool() { Get().DestroyImGuiDescriptorPoolImpl(); }
		static VmaAllocator GetAllocator() { return Get().Allocator; }
		static VkInstance GetInstance() { return Get().Instance; }
		static VkPhysicalDevice GetPhysicalDevice() { return Get().PhysicalDevice; }
		static VkDevice GetDevice() { return Get().Device; }
		static VkExtent2D GetExtent() { return Get().SwapchainExtent; }
		static VkSwapchainKHR GetSwapchain() { return Get().Swapchain; }
		static std::vector<Ref<Image>>& GetSwapchainImages() { return Get().SwapchainImages; }
		static VkFormat GetSwapchainFormat() { return Get().SwapchainFormat; }
		static VkQueue GetQueue() { return Get().Queue; }
		static VkSampleCountFlagBits GetMSAASamples() { return Get().MSAASamples; }
		static GPUInfo* GetGPUInfo() { return Get().GPU; }

		static VkCommandPool CreateCommandPool() { return Get().CreateCommandPoolImpl(); }
		static VkCommandBuffer CreateCommandBuffer(VkCommandPool commandPool) { return Get().CreateCommandBufferImpl(commandPool); }
		static VkFence CreateFence(bool signaled) { return Get().CreateFenceImpl(signaled); }
		static VkSemaphore CreateVkSemaphore() { return Get().CreateSemaphoreImpl(); }

		static void ImmediateSubmit(std::function<void(VkCommandBuffer commandBuffer)>&& function) { return Get().ImmediateSubmitImpl(std::move(function)); }
	public:
		GraphicsContext(GraphicsContext const&) = delete;
		void operator=(GraphicsContext const&) = delete;
	private:
		GraphicsContext() = default;

		void InitializeImpl();
		void DeinitializeImpl();
		void RecreateSwapChainImpl();
		void WaitIdleImpl();
		void GetImGuiDescriptorPoolImpl();
		void DestroyImGuiDescriptorPoolImpl();
		void ImmediateSubmitImpl(std::function<void(VkCommandBuffer commandBuffer)>&& function);

		VkCommandPool CreateCommandPoolImpl();
		VkCommandBuffer CreateCommandBufferImpl(VkCommandPool commandPool);
		VkFence CreateFenceImpl(bool signaled);
		VkSemaphore CreateSemaphoreImpl();

	public:
		void CreateInstance();
		void DestroyInstance();
		void SetupDebugMessenger();
		void EnumeratePhysicalDevices();
		void SelectPhysicalDevice();
		void CreateLogicalDeviceAndQueues();
		void InitializeAllocator();
		void CreateCommandPools();
		void CreateCommandBuffers();
		void CreateSwapChain();
		VkFormat ChooseSupportedFormat(VkFormat* formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features);

		void CleanupSwapChain();

	private:
		VkSampleCountFlagBits GetMaxMSAASampleCount();

	private:
		bool m_Initialized = false;

	public:
		VkInstance Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;

		GPUInfo* GPU = nullptr;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkDevice Device = VK_NULL_HANDLE;

		uint32_t QueueFamilyIndex;

		VkQueue Queue = VK_NULL_HANDLE;

		VkSurfaceKHR Surface = VK_NULL_HANDLE;

		VmaAllocator Allocator = VK_NULL_HANDLE;

		VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
		VkFormat SwapchainFormat = VK_FORMAT_UNDEFINED;

		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		VkExtent2D SwapchainExtent = { 0 };

		std::vector<Ref<Image>> SwapchainImages;

		VkCommandPool UploadCommandPool;

		VkFence UploadFence;

		VkSampleCountFlagBits MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		VkDescriptorPool ImGuiDescriptorPool;

		VkApplicationInfo ApplicationInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Test app",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Hog",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_MAKE_VERSION(1, 3, VK_HEADER_VERSION)
		};

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = DebugCallback,
			.pUserData = nullptr // Optional
		};

		// These are some features that are enabled for VkNeo
		// If you try to make an API call down the road which 
		// requires something be enabled, you'll more than likely
		// get a validation message telling you what to enable.
		// Thanks Vulkan!
		VkPhysicalDeviceFeatures DeviceFeatures =
		{
			.imageCubeArray = VK_TRUE,
			.sampleRateShading = VK_TRUE,
			.depthClamp = VK_TRUE,
			.depthBiasClamp = VK_TRUE,
			.fillModeNonSolid = VK_TRUE,
			.depthBounds = VK_TRUE,
			.samplerAnisotropy = VK_TRUE,
			.textureCompressionBC = VK_TRUE,
			.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
		};

		VkPhysicalDeviceVulkan13Features DeviceFeatures13 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.synchronization2 = VK_TRUE,
			.maintenance4 = VK_TRUE,
		};

		VkPhysicalDeviceVulkan12Features DeviceFeatures12 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.pNext = &DeviceFeatures13,
		};

		VkPhysicalDeviceVulkan11Features DeviceFeatures11 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.pNext = &DeviceFeatures12,
		};

		std::vector<const char*> InstanceExtensions = {  };
		std::vector<const char*> DeviceExtensions = { VK_KHR_MAINTENANCE_4_EXTENSION_NAME, VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME};

		std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

		std::vector<GPUInfo> GPUs;
	};
}
