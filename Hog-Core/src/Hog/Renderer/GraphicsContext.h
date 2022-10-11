#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include "Hog/Renderer/Image.h"
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

			VkPhysicalDeviceMemoryProperties2 MemoryProperties2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
			};

			VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
			};

			VkPhysicalDeviceAccelerationStructurePropertiesKHR AccelerationStructureProperties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
				.pNext = &RayTracingPipelineProperties,
			};

			VkPhysicalDeviceVulkan13Properties Vulkan13Properties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
				.pNext = &AccelerationStructureProperties,
			};

			VkPhysicalDeviceVulkan12Properties Vulkan12Properties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
				.pNext = &Vulkan13Properties,
			};

			VkPhysicalDeviceVulkan11Properties Vulkan11Properties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
				.pNext = &Vulkan12Properties,
			};

			VkPhysicalDeviceProperties2 DeviceProperties2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
				.pNext = &Vulkan11Properties,
			};

			VkPhysicalDeviceBufferAddressFeaturesEXT BufferDeviceAddressFetures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT,
			};

			VkPhysicalDeviceRayQueryFeaturesKHR RayQueryFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
				.pNext = &BufferDeviceAddressFetures,
			};

			VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
				.pNext = &RayQueryFeatures,
			};

			VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
				.pNext = &RayTracingPipelineFeatures,
			};

			VkPhysicalDeviceVulkan13Features PhysicalDeviceVulkan13Features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
				.pNext = &AccelerationStructureFeatures,
			};

			VkPhysicalDeviceVulkan12Features PhysicalDeviceVulkan12Features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
				.pNext = &PhysicalDeviceVulkan13Features,
			};

			VkPhysicalDeviceVulkan11Features PhysicalDeviceVulkan11Features = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
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
		static VkDescriptorPool GetImGuiDescriptorPool() { Get().GetImGuiDescriptorPoolImpl(); return Get().m_ImGuiDescriptorPool; }
		static void DestroyImGuiDescriptorPool() { Get().DestroyImGuiDescriptorPoolImpl(); }
		static VmaAllocator GetAllocator() { return Get().m_Allocator; }
		static VkInstance GetInstance() { return Get().m_Instance; }
		static VkPhysicalDevice GetPhysicalDevice() { return Get().m_PhysicalDevice; }
		static VkDevice GetDevice() { return Get().m_Device; }
		static VkExtent2D GetExtent() { return Get().m_SwapchainExtent; }
		static VkSwapchainKHR GetSwapchain() { return Get().m_Swapchain; }
		static std::vector<Ref<Image>>& GetSwapchainImages() { return Get().m_SwapchainImages; }
		static VkFormat GetSwapchainFormat() { return Get().m_SwapchainFormat; }
		static VkQueue GetQueue() { return Get().m_Queue; }
		static uint32_t GetQueueFamily() { return Get().m_QueueFamilyIndex; }
		static VkSampleCountFlagBits GetMSAASamples() { return Get().m_MSAASamples; }
		static GPUInfo* GetGPUInfo() { return Get().m_GPU; }

		static VkCommandPool CreateCommandPool() { return Get().CreateCommandPoolImpl(); }
		static VkCommandBuffer CreateCommandBuffer(VkCommandPool commandPool) { return Get().CreateCommandBufferImpl(commandPool); }
		static VkFence CreateFence(bool signaled) { return Get().CreateFenceImpl(signaled); }
		static VkSemaphore CreateVkSemaphore() { return Get().CreateSemaphoreImpl(); }

		static std::vector<const char*>& GetInstanceExtensions() { return Get().GetInstanceExtensionsImpl(); }

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
		VkSampleCountFlagBits GetMaxMSAASampleCount();

		std::vector<const char*>& GetInstanceExtensionsImpl() { return m_InstanceExtensions; }

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
		bool m_Initialized = false;

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		GPUInfo* m_GPU = nullptr;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_QueueFamilyIndex;

		VkQueue m_Queue = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;

		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		VkExtent2D m_SwapchainExtent = { 0 };

		std::vector<Ref<Image>> m_SwapchainImages;

		VkCommandPool m_UploadCommandPool;

		VkFence UploadFence;

		VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		VkDescriptorPool m_ImGuiDescriptorPool;

		VkApplicationInfo m_ApplicationInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Test app",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Hog",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_MAKE_VERSION(1, 3, VK_HEADER_VERSION)
		};

		VkDebugUtilsMessengerCreateInfoEXT m_DebugMessengerCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = DebugCallback,
			.pUserData = nullptr // Optional
		};

		VkPhysicalDeviceBufferAddressFeaturesEXT m_BufferDeviceAddressFetures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT,
			.bufferDeviceAddress = VK_TRUE,
		};

		VkPhysicalDeviceRayQueryFeaturesKHR m_RayQueryFeatures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
			.pNext = &m_BufferDeviceAddressFetures,
		};

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR m_RayTracingPipelineFeatures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
			.pNext = &m_RayQueryFeatures,
			.rayTracingPipeline = VK_TRUE,
		};

		VkPhysicalDeviceAccelerationStructureFeaturesKHR m_AccelerationStructureFeatures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
			.pNext = &m_RayTracingPipelineFeatures,
			.accelerationStructure = VK_TRUE,
		};

		VkPhysicalDeviceVulkan13Features m_DeviceFeatures13 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.pNext = &m_AccelerationStructureFeatures,
			.synchronization2 = VK_TRUE,
			.maintenance4 = VK_TRUE,
		};

		VkPhysicalDeviceVulkan12Features m_DeviceFeatures12 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.pNext = &m_DeviceFeatures13,
	        .descriptorIndexing = VK_TRUE,
	        .shaderInputAttachmentArrayDynamicIndexing = VK_TRUE,
	        .shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE,
	        .shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE,
	        .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
	        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
	        .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
	        .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
	        .shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE,
	        .shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE,
	        .shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE,
	        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
	        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
	        .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
	        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
	        .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE,
	        .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE,
	        .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
	        .descriptorBindingPartiallyBound = VK_TRUE,
	        .descriptorBindingVariableDescriptorCount = VK_TRUE,
	        .runtimeDescriptorArray = VK_TRUE,
			.bufferDeviceAddress = VK_TRUE,
		};

		VkPhysicalDeviceVulkan11Features m_DeviceFeatures11 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.pNext = &m_DeviceFeatures12,
			.multiview = VK_TRUE,
		};

		VkPhysicalDeviceFeatures m_DeviceFeatures =
		{
			.imageCubeArray = VK_TRUE,
			.geometryShader = VK_TRUE,
			.sampleRateShading = VK_TRUE,
			.depthClamp = VK_TRUE,
			.depthBiasClamp = VK_TRUE,
			.fillModeNonSolid = VK_TRUE,
			.depthBounds = VK_TRUE,
			.samplerAnisotropy = VK_TRUE,
			.textureCompressionBC = VK_TRUE,
			.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
		};

		std::vector<const char*> m_InstanceExtensions = {

		};

		std::vector<const char*> m_DeviceExtensions = { 
			VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, 
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
			VK_KHR_SPIRV_1_4_EXTENSION_NAME,
			VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME,
		};

		std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

		std::vector<GPUInfo> m_GPUs;
	};
}
