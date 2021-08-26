#include "vkcpch.h"
#include "GraphicsContext.h"

#include "VulkanCore/Core/Application.h"

namespace VulkanCore {
	static void CheckResult(VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			VKC_CORE_ERROR("Vulkan check failed with code: {0}", result);
			VKC_CORE_ASSERT(false)
		}
	}

	static bool CheckPhysicalDeviceExtensionSupport(GraphicsContext::GPUInfo* gpu, std::vector<const char*>& extensions)
	{
		bool supported = true;
		for (auto extension : extensions)
		{
			bool found = false;

			for (auto props : gpu->ExtensionProperties)
			{
				if (std::strcmp(props.extensionName, extension) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				supported = false;
				break;
			}
		}

		return supported;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	void GraphicsContext::InitializeImpl()
	{
		CreateInstance();
		SetupDebugMessenger();
		Application::Get().GetWindow().CreateSurface(m_Context.Instance, nullptr, &(m_Context.Surface));
		EnumeratePhysicalDevices();
		SelectPhysicalDevice();
	}

	void GraphicsContext::DeinitializeImpl()
	{
		if (m_Context.EnableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_Context.Instance, m_Context.DebugMessenger, nullptr);
		}
		
		DestroySurface();
		DestroyInstance();
	}

	void GraphicsContext::CreateInstance()
	{
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &(m_Context.ApplicationInfo);

		if (m_Context.EnableValidationLayers) {
			m_Context.InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		for (auto instanceExtension : m_Context.InstanceExtensions)
		{
			bool exists = false;
			for (const auto& extension : extensions) 
			{
				if (std::strcmp(instanceExtension, extension.extensionName) == 0)
				{
					exists = true;
				}
			}

			VKC_ASSERT(exists, "Extension not supported");
		}

		if (m_Context.EnableValidationLayers)
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (auto validationLayer : m_Context.ValidationLayers)
			{
				bool exists = false;
				for (const auto& layer : availableLayers)
				{
					if (std::strcmp(validationLayer, layer.layerName) == 0)
					{
						exists = true;
					}
				}

				VKC_ASSERT(exists, "Extension not supported");
			}

			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&m_Context.DebugMessengerCreateInfo;
		}
		else
		{
			m_Context.ValidationLayers.clear();
		}

		// Give all the extensions/layers to the create info.
		createInfo.enabledExtensionCount = (uint32_t)m_Context.InstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = m_Context.InstanceExtensions.data();
		createInfo.enabledLayerCount = (uint32_t)m_Context.ValidationLayers.size();
		createInfo.ppEnabledLayerNames = m_Context.ValidationLayers.data();

		CheckResult(vkCreateInstance(&createInfo, nullptr, &m_Context.Instance));
	}

	void GraphicsContext::DestroyInstance()
	{
		vkDestroyInstance(m_Context.Instance, nullptr);
	}

	void GraphicsContext::DestroySurface()
	{
		vkDestroySurfaceKHR(m_Context.Instance, m_Context.Surface, nullptr);
	}

	void GraphicsContext::SetupDebugMessenger()
	{
		if (!m_Context.EnableValidationLayers) return;
		
		CheckResult(CreateDebugUtilsMessengerEXT(m_Context.Instance, &m_Context.DebugMessengerCreateInfo, nullptr, &m_Context.DebugMessenger));
	}

	void GraphicsContext::EnumeratePhysicalDevices()
	{
		// CheckResult and VKC_ASSERT are simply macros for checking return values,
		// and then taking action if necessary.
	
		// First just get the number of devices.
		uint32_t numDevices = 0;
		CheckResult(vkEnumeratePhysicalDevices(m_Context.Instance, &numDevices, nullptr));
		VKC_ASSERT(numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices.")

		std::vector<VkPhysicalDevice> devices(numDevices);

		// Now get the actual devices
		CheckResult(vkEnumeratePhysicalDevices(m_Context.Instance, &numDevices, devices.data()));
		VKC_ASSERT(numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices.")

		// GPU is a VkNeo struct which stores details about the physical device.
		// We'll use various API calls to get the necessary information.
		m_Context.GPUs.resize(numDevices);

		for (uint32_t i = 0; i < numDevices; ++i) {
			GraphicsContext::GPUInfo& gpu = m_Context.GPUs[i];
			gpu.Device = devices[i];

			{
				// First let's get the Queues from the device.  
				// DON'T WORRY I'll explain this in a bit.
				uint32_t numQueues = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.Device, &numQueues, nullptr);
				VKC_ASSERT(numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues.");

				gpu.QueueFamilyProperties.resize(numQueues);
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.Device, &numQueues, gpu.QueueFamilyProperties.data());
				VKC_ASSERT(numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues.");
			}

			{
				// Next let's get the extensions supported by the device.
				uint32_t numExtension;
				CheckResult(vkEnumerateDeviceExtensionProperties(gpu.Device, nullptr, &numExtension, nullptr));
				VKC_ASSERT(numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions.");

				gpu.ExtensionProperties.resize(numExtension);
				CheckResult(vkEnumerateDeviceExtensionProperties(gpu.Device, nullptr, &numExtension, gpu.ExtensionProperties.data()));
				VKC_ASSERT(numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions.");
			}

			// Surface capabilities basically describes what kind of image you can render to the user.
			// Look up VkSurfaceCapabilitiesKHR in the Vulkan documentation.
			CheckResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.Device, m_Context.Surface, &gpu.SurfaceCapabilities));

			{
				// Get the supported surface formats.  This includes image format and color space.
				// A common format is VK_FORMAT_R8G8B8A8_UNORM which is 8 bits for red, green, blue, alpha making for 32 total
				uint32_t numFormats;
				CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Device, m_Context.Surface, &numFormats, nullptr));
				VKC_ASSERT(numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats.");

				gpu.SurfaceFormats.resize(numFormats);
				CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Device, m_Context.Surface, &numFormats, gpu.SurfaceFormats.data()));
				VKC_ASSERT(numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats.");
			}

			{
				// Vulkan supports multiple presentation modes, and I'll linkn to some good documentation on that in just a bit.
				uint32_t numPresentModes;
				CheckResult(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Device, m_Context.Surface, &numPresentModes, nullptr));
				VKC_ASSERT(numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes.");

				gpu.PresentModes.resize(numPresentModes);
				CheckResult(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Device, m_Context.Surface, &numPresentModes, gpu.PresentModes.data()));
				VKC_ASSERT(numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes.");
			}

			// Almost done! Up next wee get the memory types supported by the device.
			// This will be needed later once we start allocating memory for buffers, images, etc.
			vkGetPhysicalDeviceMemoryProperties(gpu.Device, &gpu.MemoryProperties);

			// Lastly we get the actual device properties.
			// Of note this includes a MASSIVE struct (VkPhysicalDeviceLimits) which outlines 
			// all possible limits you could run into when attemptin to render.
			vkGetPhysicalDeviceProperties(gpu.Device, &gpu.DeviceProperties);
		}
	}

	void GraphicsContext::SelectPhysicalDevice()
	{
		// Let's pick a GPU!
		for (auto gpu : m_Context.GPUs)
		{
			// This is again related to queues.  Don't worry I'll get there soon.
			int graphicsIdx = -1;
			int presentIdx = -1;

			// Remember when we created our instance we got all those device extensions?
			// Now we need to make sure our physical device supports them.
			if (!CheckPhysicalDeviceExtensionSupport(&gpu, m_Context.DeviceExtensions)) {
				continue;
			}

			// No surface formats? =(
			if (gpu.SurfaceFormats.empty()) {
				continue;
			}

			// No present modes? =(
			if (gpu.PresentModes.empty()) {
				continue;
			}

			// Now we'll loop through the queue family properties looking
			// for both a graphics and a present queue.
			// The index could actually end up being the same, and from 
			// my experience they are.  But via the spec, you're not 
			// guaranteed that luxury.  So best be on the safe side.

			// Find graphics queue family
			for (int j = 0; j < gpu.QueueFamilyProperties.size(); ++j) {
				VkQueueFamilyProperties& props = gpu.QueueFamilyProperties[j];

				if (props.queueCount == 0) {
					continue;
				}

				if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					// Got it!
					graphicsIdx = j;
					break;
				}
			}

			// Find present queue family
			for (int j = 0; j < gpu.QueueFamilyProperties.size(); ++j) {
				VkQueueFamilyProperties& props = gpu.QueueFamilyProperties[j];

				if (props.queueCount == 0) {
					continue;
				}

				// A rather perplexing call in the Vulkan API, but
				// it is a necessity to call.
				VkBool32 supportsPresent = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(gpu.Device, j, m_Context.Surface, &supportsPresent);
				if (supportsPresent) {
					// Got it!
					presentIdx = j;
					break;
				}
			}

			// Did we find a device supporting both graphics and present.
			if (graphicsIdx >= 0 && presentIdx >= 0) {
				m_Context.GraphicsFamilyIndex = (uint32_t)graphicsIdx;
				m_Context.PresentFamilyIndex = (uint32_t)presentIdx;
				m_Context.PhysicalDevice = gpu.Device;
				m_Context.GPU = &gpu;
				return;
			}
		}

		// If we can't render or present, just bail.
		// DIAF
		VKC_CORE_ERROR("Could not find a physical device which fits our desired profile");
		VKC_CORE_ASSERT(false)
	}
}
