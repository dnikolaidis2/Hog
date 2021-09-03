#pragma once

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
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
		enum class TextureFormat
		{
			FORMAT_RGBA8,
			FORMAT_DEPTH
		};

		enum class TextureType
		{
			TYPE_2D,
			TYPE_CUBIC
		};

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

		struct VKImage
		{
			VkImage Image;
			VkImageView View;
			VkFormat InternalFormat;
			struct Options
			{
				TextureType Type = TextureType::TYPE_2D;
				TextureFormat Format = TextureFormat::FORMAT_RGBA8;
				uint32_t LevelCount = 1;
				uint32_t Width;
				uint32_t Height;
			} Options;
			
			bool IsSwapChainImage;

			VKImage() = default;

			VKImage(const VKImage&) = default;

			VKImage(VkImage image, VkImageView imageView, VkFormat format, VkExtent2D extent)
				: Image(image), View(imageView), InternalFormat(format)
			{
				Options.Width = extent.width;
				Options.Height = extent.height;
			}
		};

		struct ContextData
		{
			int FrameCount = 2;
			int CurrentFrame = 0;

			VkApplicationInfo ApplicationInfo = 
			{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Test app",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "Hazel",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_MAKE_VERSION(1, 2, VK_HEADER_VERSION)
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
				.depthClamp = VK_TRUE,
				.depthBiasClamp = VK_TRUE,
				.fillModeNonSolid = VK_TRUE,
				.depthBounds = VK_TRUE,
				.textureCompressionBC = VK_TRUE
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

			VkDevice Device = VK_NULL_HANDLE;
			VkQueue GraphicsQueue;
			VkQueue PresentQueue;

			VmaAllocator Allocator;

			VkSurfaceKHR Surface = VK_NULL_HANDLE;

			std::vector<VkSemaphore> AcquireSemaphores;
			std::vector<VkSemaphore> RenderCompleteSemaphores;

			VkCommandPool CommandPool;

			std::vector<VkCommandBuffer> CommandBuffers;
			std::vector<VkFence> CommandBufferFences;

			VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

			VkFormat SwapchainFormat;
			VkPresentModeKHR PresentMode;
			VkExtent2D SwapchainExtent;

			std::vector<VKImage> SwapchainImages;

			VkFormat DepthFormat;
			VKImage DepthImage;

			VkRenderPass RenderPass;

			std::vector<VkFramebuffer> FrameBuffers;
		};

	public:
		static GraphicsContext& Get()
		{
			static GraphicsContext instance;

			return instance;
		}

		~GraphicsContext() { Get().Deinitialize(); }

		static void Initialize() { Get().InitializeImpl(); }
		static void Deinitialize() { Get().DeinitializeImpl(); }
		static void RecreateSwapChain() { Get().RecreateSwapChainImpl(); }
		static ContextData& GetContext() { return Get().m_Context; }
		static VkInstance& GetInstance() { return Get().m_Context.Instance; }
		static VkDevice GetDevice() { return Get().m_Context.Device; }
		static int GetCurrentFrame() { return Get().m_Context.CurrentFrame; }
		static void SetCurrentFrame(int frame) { Get().m_Context.CurrentFrame = frame; }
	public:
		GraphicsContext(GraphicsContext const&) = delete;
		void operator=(GraphicsContext const&) = delete;
	private:
		GraphicsContext() = default;

		void InitializeImpl();
		void DeinitializeImpl();
		void RecreateSwapChainImpl();

		void CreateInstance();
		void DestroyInstance();
		void SetupDebugMessenger();
		void EnumeratePhysicalDevices();
		void SelectPhysicalDevice();
		void CreateLogicalDeviceAndQueues();
		void InitializeAllocator();
		void CreateSemaphores();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSwapChain();
		VkFormat ChooseSupportedFormat(VkFormat* formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features);
		void CreateRenderTargets();
		void CreateRenderPass();
		void CreateFrameBuffers();

		void CleanupSwapChain();
	private:
		ContextData m_Context;
	};
}