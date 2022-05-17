#include "hgpch.h"
#include "GraphicsContext.h"

#include "vk_mem_alloc.h"
#include "Hog/Core/CVars.h"
#include "Hog/Core/Application.h"
#include "Hog/Utils/RendererUtils.h"

AutoCVar_Int CVar_MSAA("renderer.enableMSAA", "Enables MSAA for renderer", 0, CVarFlags::EditReadOnly);
AutoCVar_Int CVar_ValidationLayers("renderer.enableValidationLayers", "Enables Vulkan validation layers", 1, CVarFlags::EditReadOnly);
AutoCVar_Int CVar_FrameCount("renderer.frameCount", "Number off frames being rendered", 2, CVarFlags::EditReadOnly);

namespace Hog {

	static bool CheckPhysicalDeviceFeatureSupport(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceFeatures features);

	static bool CheckPhysicalDeviceFeatureSupport11(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceVulkan11Features features);

	static bool CheckPhysicalDeviceFeatureSupport12(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceVulkan12Features features);

	static bool CheckPhysicalDeviceFeatureSupport13(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceVulkan13Features  features);

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

	static VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats)
	{
		VkSurfaceFormatKHR result;

		// If Vulkan returned an unknown format, then just force what we want.
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		{
			result.format = VK_FORMAT_B8G8R8A8_SRGB;
			result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			return result;
		}

		// Favor 32 bit rgba and srgb nonlinear colorspace
		for (int i = 0; i < formats.size(); ++i)
		{
			VkSurfaceFormatKHR& fmt = formats[i];
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return fmt;
			}
		}

		// If all else fails, just return what's available
		return formats[0];
	}

	static VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR>& modes)
	{
		const VkPresentModeKHR desiredMode = VK_PRESENT_MODE_MAILBOX_KHR;

		// Favor looking for mailbox mode.
		for (int i = 0; i < modes.size(); ++i)
		{
			if (modes[i] == desiredMode)
			{
				return desiredMode;
			}
		}

		// If we couldn't find mailbox, then default to FIFO which is always available.
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	static VkExtent2D ChooseSurfaceExtent(VkSurfaceCapabilitiesKHR& caps)
	{
		if (caps.currentExtent.width != UINT32_MAX) {
			return caps.currentExtent;
		}
		else {
			int width = Application::Get().GetWindow().GetFrameBufferWidth(),
				height = Application::Get().GetWindow().GetFrameBufferHeight();


			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	void GraphicsContext::InitializeImpl()
	{
		HG_PROFILE_FUNCTION();

#if HG_PROFILE
		CVar_ValidationLayers.Set(0);
#endif

		CreateInstance();
		SetupDebugMessenger();
		Application::Get().GetWindow().CreateSurface(Instance, nullptr, &(Surface));
		EnumeratePhysicalDevices();
		SelectPhysicalDevice();
		CreateLogicalDeviceAndQueues();
		InitializeAllocator();
		CreateCommandPools();
		CreateCommandBuffers();
		CreateSwapChain();

		m_Initialized = true;
	}

	void GraphicsContext::DeinitializeImpl()
	{
		for (auto& image : SwapchainImages)
		{
			image.reset();
		}

		vkDestroySwapchainKHR(Device, Swapchain, nullptr);

		vkDestroyFence(Device, UploadFence, nullptr);

		vkDestroyCommandPool(Device, UploadCommandPool, nullptr);

		vmaDestroyAllocator(Allocator);

		vkDestroyDevice(Device, nullptr);
		vkDestroySurfaceKHR(Instance, Surface, nullptr);

		if (CVar_ValidationLayers.Get())
		{
			DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
		}

		DestroyInstance();

		m_Initialized = false;
	}

	void GraphicsContext::RecreateSwapChainImpl()
	{
		vkDeviceWaitIdle(Device);

		CleanupSwapChain();

		CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPU->Device, Surface, &GPU->SurfaceCapabilities));
		CreateSwapChain();
	}

	void GraphicsContext::WaitIdleImpl()
	{
		vkDeviceWaitIdle(Device);
	}

	void GraphicsContext::GetImGuiDescriptorPoolImpl()
	{
		if (ImGuiDescriptorPool) return;

		//1: create descriptor pool for IMGUI
		// the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		CheckVkResult(vkCreateDescriptorPool(Device, &pool_info, nullptr, &ImGuiDescriptorPool));
	}

	void GraphicsContext::DestroyImGuiDescriptorPoolImpl()
	{
		vkDestroyDescriptorPool(Device, ImGuiDescriptorPool, nullptr);
	}

	void GraphicsContext::ImmediateSubmitImpl(std::function<void(VkCommandBuffer commandBuffer)>&& function)
	{
		//allocate the default command buffer that we will use for the instant commands
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		// Don't worry about this
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		// The command pool we created above
		commandBufferAllocateInfo.commandPool = UploadCommandPool;

		// We'll have two command buffers.  One will be in flight
		// while the other is being built.
		commandBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		CheckVkResult(vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, &commandBuffer));

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		CheckVkResult(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		HG_PROFILE_GPU_CONTEXT(commandBuffer);
		HG_PROFILE_GPU_EVENT("Immediate Submit");

		//execute the function
		function(commandBuffer);

		CheckVkResult(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//submit command buffer to the queue and execute it.
		// _uploadFence will now block until the graphic commands finish execution
		CheckVkResult(vkQueueSubmit(GraphicsQueue, 1, &submitInfo, UploadFence));

		vkWaitForFences(Device, 1, &UploadFence, true, 9999999999);
		vkResetFences(Device, 1, &UploadFence);

		//clear the command pool. This will free the command buffer too
		vkResetCommandPool(Device, UploadCommandPool, 0);
	}

	VkFence GraphicsContext::CreateFenceImpl(bool signaled)
	{
		VkFence fence;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		CheckVkResult(vkCreateFence(Device, &fenceCreateInfo, nullptr, &fence));

		return fence;
	}

	VkCommandPool GraphicsContext::CreateCommandPoolImpl()
	{
		VkCommandPool commandPool;

		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;

		CheckVkResult(vkCreateCommandPool(Device, &commandPoolCreateInfo, nullptr, &commandPool));

		return commandPool;
	}

	VkCommandBuffer GraphicsContext::CreateCommandBufferImpl(VkCommandPool commandPool)
	{
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.commandBufferCount = 1;

		CheckVkResult(vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, &commandBuffer));

		return commandBuffer;
	}

	void GraphicsContext::CreateInstance()
	{
		HG_PROFILE_FUNCTION();

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &(ApplicationInfo);

		if (CVar_ValidationLayers.Get()) {
			InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		for (auto instanceExtension : InstanceExtensions)
		{
			bool exists = false;
			for (const auto& extension : extensions)
			{
				if (std::strcmp(instanceExtension, extension.extensionName) == 0)
				{
					exists = true;
					break;
				}
			}

			HG_ASSERT(exists, "Extension not supported");
		}

		if (CVar_ValidationLayers.Get())
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (auto validationLayer : ValidationLayers)
			{
				bool exists = false;
				for (const auto& layer : availableLayers)
				{
					if (std::strcmp(validationLayer, layer.layerName) == 0)
					{
						exists = true;
						break;
					}
				}

				HG_ASSERT(exists, "Extension not supported");
			}

			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&DebugMessengerCreateInfo;
		}
		else
		{
			ValidationLayers.clear();
		}

		// Give all the extensions/layers to the create info.
		createInfo.enabledExtensionCount = (uint32_t)InstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = InstanceExtensions.data();
		createInfo.enabledLayerCount = (uint32_t)ValidationLayers.size();
		createInfo.ppEnabledLayerNames = ValidationLayers.data();

		CheckVkResult(vkCreateInstance(&createInfo, nullptr, &Instance));
	}

	void GraphicsContext::DestroyInstance()
	{
		vkDestroyInstance(Instance, nullptr);
	}

	void GraphicsContext::SetupDebugMessenger()
	{
		HG_PROFILE_FUNCTION();
		if (!CVar_ValidationLayers.Get()) return;

		CheckVkResult(CreateDebugUtilsMessengerEXT(Instance, &DebugMessengerCreateInfo, nullptr, &DebugMessenger));
	}

	void GraphicsContext::EnumeratePhysicalDevices()
	{
		HG_PROFILE_FUNCTION();
		// CheckVkResult and HG_ASSERT are simply macros for checking return values,
		// and then taking action if necessary.

		// First just get the number of devices.
		uint32_t numDevices = 0;
		CheckVkResult(vkEnumeratePhysicalDevices(Instance, &numDevices, nullptr));
		HG_ASSERT(numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices.")

			std::vector<VkPhysicalDevice> devices(numDevices);

		// Now get the actual devices
		CheckVkResult(vkEnumeratePhysicalDevices(Instance, &numDevices, devices.data()));
		HG_ASSERT(numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices.")

			// GPU is a VkNeo struct which stores details about the physical device.
			// We'll use various API calls to get the necessary information.
			GPUs.resize(numDevices);

		for (uint32_t i = 0; i < numDevices; ++i)
		{
			GPUInfo& gpu = GPUs[i];
			gpu.Device = devices[i];

			{
				// First let's get the Queues from the device.  
				// DON'T WORRY I'll explain this in a bit.
				uint32_t numQueues = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.Device, &numQueues, nullptr);
				HG_ASSERT(numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues.");

				gpu.QueueFamilyProperties.resize(numQueues);
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.Device, &numQueues, gpu.QueueFamilyProperties.data());
				HG_ASSERT(numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues.");
			}

			{
				// Next let's get the extensions supported by the device.
				uint32_t numExtension;
				CheckVkResult(vkEnumerateDeviceExtensionProperties(gpu.Device, nullptr, &numExtension, nullptr));
				HG_ASSERT(numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions.");

				gpu.ExtensionProperties.resize(numExtension);
				CheckVkResult(vkEnumerateDeviceExtensionProperties(gpu.Device, nullptr, &numExtension, gpu.ExtensionProperties.data()));
				HG_ASSERT(numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions.");
			}

			// Surface capabilities basically describes what kind of image you can render to the user.
			// Look up VkSurfaceCapabilitiesKHR in the Vulkan documentation.
			CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.Device, Surface, &gpu.SurfaceCapabilities));

			{
				// Get the supported surface formats.  This includes image format and color space.
				// A common format is VK_FORMAT_R8G8B8A8_UNORM which is 8 bits for red, green, blue, alpha making for 32 total
				uint32_t numFormats;
				CheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Device, Surface, &numFormats, nullptr));
				HG_ASSERT(numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats.");

				gpu.SurfaceFormats.resize(numFormats);
				CheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Device, Surface, &numFormats, gpu.SurfaceFormats.data()));
				HG_ASSERT(numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats.");
			}

			{
				// Vulkan supports multiple presentation modes, and I'll linkn to some good documentation on that in just a bit.
				uint32_t numPresentModes;
				CheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Device, Surface, &numPresentModes, nullptr));
				HG_ASSERT(numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes.");

				gpu.PresentModes.resize(numPresentModes);
				CheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Device, Surface, &numPresentModes, gpu.PresentModes.data()));
				HG_ASSERT(numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes.");
			}

			{
				// Get physical device features to check with features required by application
				vkGetPhysicalDeviceFeatures(gpu.Device, &gpu.PhysicalDeviceFeatures);

				vkGetPhysicalDeviceFeatures2(gpu.Device, &gpu.PhysicalDeviceFeatures2);
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
		HG_PROFILE_FUNCTION();
		// Let's pick a GPU!
		for (uint32_t i = 0; i < GPUs.size(); i++)
		{
			GPUInfo* gpu = &(GPUs[i]);
			// This is again related to queues.  Don't worry I'll get there soon.
			int graphicsIdx = -1;
			int computeIdx = -1;
			int presentIdx = -1;

			// Remember when we created our instance we got all those device extensions?
			// Now we need to make sure our physical device supports them.
			if (!CheckPhysicalDeviceExtensionSupport(gpu, DeviceExtensions))
			{
				continue;
			}

			// No surface formats? =(
			if (gpu->SurfaceFormats.empty())
			{
				continue;
			}

			// No present modes? =(
			if (gpu->PresentModes.empty())
			{
				continue;
			}

			// Now we'll loop through the queue family properties looking
			// for both a graphics and a present queue.
			// The index could actually end up being the same, and from 
			// my experience they are.  But via the spec, you're not 
			// guaranteed that luxury.  So best be on the safe side.

			// Find graphics queue family
			for (uint32_t j = 0; j < gpu->QueueFamilyProperties.size(); ++j)
			{
				VkQueueFamilyProperties& props = gpu->QueueFamilyProperties[j];

				if (props.queueCount == 0)
				{
					continue;
				}

				if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					// Got it!
					graphicsIdx = j;
					break;
				}
			}

			// Find compute queue family
			for (uint32_t j = 0; j < gpu->QueueFamilyProperties.size(); ++j)
			{
				VkQueueFamilyProperties& props = gpu->QueueFamilyProperties[j];

				if (props.queueCount == 0)
				{
					continue;
				}

				if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					// Got it!
					computeIdx = j;
					break;
				}
			}

			// Find present queue family
			for (int j = 0; j < gpu->QueueFamilyProperties.size(); ++j)
			{
				VkQueueFamilyProperties& props = gpu->QueueFamilyProperties[j];

				if (props.queueCount == 0)
				{
					continue;
				}

				// A rather perplexing call in the Vulkan API, but
				// it is a necessity to call.
				VkBool32 supportsPresent = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(gpu->Device, j, Surface, &supportsPresent);
				if (supportsPresent)
				{
					// Got it!
					presentIdx = j;
					break;
				}
			}

			// Check that all features are supported.
			if (!CheckPhysicalDeviceFeatureSupport(gpu, DeviceFeatures))
			{
				continue;
			}

			if (!CheckPhysicalDeviceFeatureSupport11(gpu, DeviceFeatures11))
			{
				continue;
			}

			if (!CheckPhysicalDeviceFeatureSupport12(gpu, DeviceFeatures12))
			{
				continue;
			}

			if (!CheckPhysicalDeviceFeatureSupport13(gpu, DeviceFeatures13))
			{
				continue;
			}

			// Did we find a device supporting both graphics and present.
			if (graphicsIdx >= 0 && presentIdx >= 0)
			{
				GraphicsFamilyIndex = (uint32_t)graphicsIdx;
				PresentFamilyIndex = (uint32_t)presentIdx;
				ComputeFamilyIndex = (uint32_t)computeIdx;
				PhysicalDevice = gpu->Device;
				GPU = gpu;
				if (CVar_MSAA.Get())
				{
					MSAASamples = GetMaxMSAASampleCount();
				}
				return;
			}
		}

		// If we can't render or present, just bail.
		// DIAF
		HG_CORE_ERROR("Could not find a physical device which fits our desired profile");
		HG_CORE_ASSERT(false)
	}

	void GraphicsContext::CreateLogicalDeviceAndQueues()
	{
		HG_PROFILE_FUNCTION();
		// Add each family index to a list.
		// Don't do duplicates
		std::vector<uint32_t> uniqueIdx;
		uniqueIdx.push_back(GraphicsFamilyIndex);
		if (std::find(uniqueIdx.begin(), uniqueIdx.end(), PresentFamilyIndex) == uniqueIdx.end()) {
			uniqueIdx.push_back(PresentFamilyIndex);
		}
		if (std::find(uniqueIdx.begin(), uniqueIdx.end(), ComputeFamilyIndex) == uniqueIdx.end()) {
			uniqueIdx.push_back(ComputeFamilyIndex);
		}

		std::vector<VkDeviceQueueCreateInfo> devqInfo;

		const float priority = 1.0f;
		for (int i = 0; i < uniqueIdx.size(); ++i)
		{
			VkDeviceQueueCreateInfo qinfo = {};
			qinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			qinfo.queueFamilyIndex = uniqueIdx[i];
			qinfo.queueCount = 1;

			// Don't worry about priority
			qinfo.pQueuePriorities = &priority;

			devqInfo.push_back(qinfo);
		}

		// Put it all together.
		VkDeviceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.pNext = &DeviceFeatures11;
		info.queueCreateInfoCount = (uint32_t)devqInfo.size();
		info.pQueueCreateInfos = devqInfo.data();
		info.pEnabledFeatures = &DeviceFeatures;
		info.enabledExtensionCount = (uint32_t)DeviceExtensions.size();
		info.ppEnabledExtensionNames = DeviceExtensions.data();

		// If validation layers are enabled supply them here.
		if (CVar_ValidationLayers.Get()) {
			info.enabledLayerCount = (uint32_t)ValidationLayers.size();
			info.ppEnabledLayerNames = ValidationLayers.data();
		}
		else {
			info.enabledLayerCount = 0;
		}

		// Create the device
		CheckVkResult(vkCreateDevice(PhysicalDevice, &info, nullptr, &Device));

		// Now get the queues from the devie we just created.
		vkGetDeviceQueue(Device, GraphicsFamilyIndex, 0, &GraphicsQueue);
		vkGetDeviceQueue(Device, PresentFamilyIndex, 0, &PresentQueue);
		vkGetDeviceQueue(Device, ComputeFamilyIndex, 0, &ComputeQueue);
	}

	void GraphicsContext::InitializeAllocator()
	{
		HG_PROFILE_FUNCTION();
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;

		vmaCreateAllocator(&allocatorInfo, &Allocator);
	}

	void GraphicsContext::CreateCommandPools()
	{
		HG_PROFILE_FUNCTION();

		// Because command buffers can be very flexible, we don't want to be 
		// doing a lot of allocation while we're trying to render.
		// For this reason we create a pool to hold allocated command buffers.
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		// This allows the command buffer to be implicitly reset when vkBeginCommandBuffer is called.
		// You can also explicitly call vkResetCommandBuffer.  
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		// We'll be building command buffers to send to the graphics queue
		commandPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;

		CheckVkResult(vkCreateCommandPool(Device, &commandPoolCreateInfo, nullptr, &UploadCommandPool));
	}

	void GraphicsContext::CreateCommandBuffers()
	{
		HG_PROFILE_FUNCTION();

		// We create fences that we can use to wait for a 
		// given command buffer to be done on the GPU.
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		CheckVkResult(vkCreateFence(Device, &fenceCreateInfo, nullptr, &UploadFence));
	}

	void GraphicsContext::CreateSwapChain()
	{
		HG_PROFILE_FUNCTION();
		GPUInfo& gpu = *GPU;

		// Take our selected gpu and pick three things.
		// 1.) Surface format as described earlier.
		// 2.) Present mode. Again refer to documentation I shared.
		// 3.) Surface extent is basically just the size ( width, height ) of the image.
		VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(gpu.SurfaceFormats);
		VkPresentModeKHR presentMode = ChoosePresentMode(gpu.PresentModes);
		VkExtent2D extent = ChooseSurfaceExtent(gpu.SurfaceCapabilities);

		VkSwapchainCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = Surface;

		// double buffer again!
		info.minImageCount = CVar_FrameCount.Get();

		info.imageFormat = surfaceFormat.format;
		info.imageColorSpace = surfaceFormat.colorSpace;
		info.imageExtent = extent;
		info.imageArrayLayers = 1;

		// Aha! Something new.  There are only 8 potential bits that can be used here
		// and I'm using two.  Essentially this is what they mean.
		// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT - This is a color image I'm rendering into.
		// VK_IMAGE_USAGE_TRANSFER_SRC_BIT - I'll be copying this image somewhere. ( screenshot, postprocess )
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		// Moment of truth.  If the graphics queue family and present family don't match
		// then we need to create the swapchain with different information.
		if (GraphicsFamilyIndex != PresentFamilyIndex) {
			uint32_t indices[] = { GraphicsFamilyIndex, PresentFamilyIndex };

			// There are only two sharing modes.  This is the one to use
			// if images are not exclusive to one queue.
			info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			info.queueFamilyIndexCount = 2;
			info.pQueueFamilyIndices = indices;
		}
		else {
			// If the indices are the same, then the queue can have exclusive
			// access to the images.
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		// We just want to leave the image as is.
		info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode = presentMode;

		// Is Vulkan allowed to discard operations outside of the renderable space?
		info.clipped = VK_TRUE;

		info.oldSwapchain = Swapchain;

		// Create the swapchain
		CheckVkResult(vkCreateSwapchainKHR(Device, &info, nullptr, &Swapchain));

		// Save off swapchain details
		SwapchainFormat = surfaceFormat.format;
		PresentMode = presentMode;
		SwapchainExtent = extent;

		// Retrieve the swapchain images from the device.
		// Note that VkImage is simply a handle like everything else.

		// First call gets numImages.
		uint32_t numImages = 0;
		std::vector<VkImage> swapchainImages(CVar_FrameCount.Get());
		CheckVkResult(vkGetSwapchainImagesKHR(Device, Swapchain, &numImages, nullptr));
		HG_ASSERT(numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count.")

			// Second call uses numImages
			CheckVkResult(vkGetSwapchainImagesKHR(Device, Swapchain, &numImages, swapchainImages.data()));
		HG_ASSERT(numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count.");

		SwapchainImages.resize(CVar_FrameCount.Get());

		// New concept - Image Views
		// Much like the logical device is an interface to the physical device,
		// image views are interfaces to actual images.  Think of it as this.
		// The image exists outside of you.  But the view is your personal view 
		// ( how you perceive ) the image.
		for (int i = 0; i < CVar_FrameCount.Get(); ++i) {
			VkImageViewCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			// We don't need to swizzle ( swap around ) any of the 
			// color channels
			info.components.r = VK_COMPONENT_SWIZZLE_R;
			info.components.g = VK_COMPONENT_SWIZZLE_G;
			info.components.b = VK_COMPONENT_SWIZZLE_B;
			info.components.a = VK_COMPONENT_SWIZZLE_A;

			// There are only 4x aspect bits.  And most people will only use 3x.
			// These determine what is affected by your image operations.
			// VK_IMAGE_ASPECT_COLOR_BIT
			// VK_IMAGE_ASPECT_DEPTH_BIT
			// VK_IMAGE_ASPECT_STENCIL_BIT

			// For beginners - a base mip level of zero is par for the course.
			info.subresourceRange.baseMipLevel = 0;

			// Level count is the # of images visible down the mip chain.
			// So basically just 1...
			info.subresourceRange.levelCount = 1;
			// We don't have multiple layers to these images.
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.layerCount = 1;
			info.flags = 0;

			SwapchainImages[i] = Image::CreateSwapChainImage(swapchainImages[i], ImageType::RenderTarget, SwapchainFormat, SwapchainExtent, info);
		}
	}

	VkFormat GraphicsContext::ChooseSupportedFormat(VkFormat* formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		HG_PROFILE_FUNCTION();
		for (int i = 0; i < numFormats; ++i)
		{
			VkFormat format = formats[i];

			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		HG_CORE_ERROR("Failed to find a supported format.");

		return VK_FORMAT_UNDEFINED;
	}

	void GraphicsContext::CleanupSwapChain()
	{
		HG_PROFILE_FUNCTION();

		for (size_t i = 0; i < SwapchainImages.size(); i++) {
			SwapchainImages[i].reset();
		}
	}

	VkSampleCountFlagBits GraphicsContext::GetMaxMSAASampleCount()
	{
		const VkSampleCountFlags counts = GPU->DeviceProperties.limits.framebufferColorSampleCounts & GPU->DeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	static bool CheckPhysicalDeviceFeatureSupport(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceFeatures features)
	{
		bool supported = true;
		if (features.robustBufferAccess == VK_TRUE && gpu->PhysicalDeviceFeatures.robustBufferAccess == VK_FALSE)
		{
			HG_CORE_ERROR("robustBufferAccess feature not supported by physical device.");
			supported = false;
		}

		if (features.fullDrawIndexUint32 == VK_TRUE && gpu->PhysicalDeviceFeatures.fullDrawIndexUint32 == VK_FALSE)
		{
			HG_CORE_ERROR("fullDrawIndexUint32 feature not supported by physical device.");
			supported = false;
		}

		if (features.imageCubeArray == VK_TRUE && gpu->PhysicalDeviceFeatures.imageCubeArray == VK_FALSE)
		{
			HG_CORE_ERROR("imageCubeArray feature not supported by physical device.");
			supported = false;
		}

		if (features.independentBlend == VK_TRUE && gpu->PhysicalDeviceFeatures.independentBlend == VK_FALSE)
		{
			HG_CORE_ERROR("independentBlend feature not supported by physical device.");
			supported = false;
		}

		if (features.geometryShader == VK_TRUE && gpu->PhysicalDeviceFeatures.geometryShader == VK_FALSE)
		{
			HG_CORE_ERROR("geometryShader feature not supported by physical device.");
			supported = false;
		}

		if (features.tessellationShader == VK_TRUE && gpu->PhysicalDeviceFeatures.tessellationShader == VK_FALSE)
		{
			HG_CORE_ERROR("tessellationShader feature not supported by physical device.");
			supported = false;
		}

		if (features.sampleRateShading == VK_TRUE && gpu->PhysicalDeviceFeatures.sampleRateShading == VK_FALSE)
		{
			HG_CORE_ERROR("sampleRateShading feature not supported by physical device.");
			supported = false;
		}

		if (features.dualSrcBlend == VK_TRUE && gpu->PhysicalDeviceFeatures.dualSrcBlend == VK_FALSE)
		{
			HG_CORE_ERROR("dualSrcBlend feature not supported by physical device.");
			supported = false;
		}

		if (features.logicOp == VK_TRUE && gpu->PhysicalDeviceFeatures.logicOp == VK_FALSE)
		{
			HG_CORE_ERROR("logicOp feature not supported by physical device.");
			supported = false;
		}

		if (features.multiDrawIndirect == VK_TRUE && gpu->PhysicalDeviceFeatures.multiDrawIndirect == VK_FALSE)
		{
			HG_CORE_ERROR("multiDrawIndirect feature not supported by physical device.");
			supported = false;
		}

		if (features.drawIndirectFirstInstance == VK_TRUE && gpu->PhysicalDeviceFeatures.drawIndirectFirstInstance == VK_FALSE)
		{
			HG_CORE_ERROR("drawIndirectFirstInstance feature not supported by physical device.");
			supported = false;
		}

		if (features.depthClamp == VK_TRUE && gpu->PhysicalDeviceFeatures.depthClamp == VK_FALSE)
		{
			HG_CORE_ERROR("depthClamp feature not supported by physical device.");
			supported = false;
		}

		if (features.depthBiasClamp == VK_TRUE && gpu->PhysicalDeviceFeatures.depthBiasClamp == VK_FALSE)
		{
			HG_CORE_ERROR("depthBiasClamp feature not supported by physical device.");
			supported = false;
		}

		if (features.fillModeNonSolid == VK_TRUE && gpu->PhysicalDeviceFeatures.fillModeNonSolid == VK_FALSE)
		{
			HG_CORE_ERROR("fillModeNonSolid feature not supported by physical device.");
			supported = false;
		}

		if (features.depthBounds == VK_TRUE && gpu->PhysicalDeviceFeatures.depthBounds == VK_FALSE)
		{
			HG_CORE_ERROR("depthBounds feature not supported by physical device.");
			supported = false;
		}

		if (features.wideLines == VK_TRUE && gpu->PhysicalDeviceFeatures.wideLines == VK_FALSE)
		{
			HG_CORE_ERROR("wideLines feature not supported by physical device.");
			supported = false;
		}

		if (features.largePoints == VK_TRUE && gpu->PhysicalDeviceFeatures.largePoints == VK_FALSE)
		{
			HG_CORE_ERROR("largePoints feature not supported by physical device.");
			supported = false;
		}

		if (features.alphaToOne == VK_TRUE && gpu->PhysicalDeviceFeatures.alphaToOne == VK_FALSE)
		{
			HG_CORE_ERROR("alphaToOne feature not supported by physical device.");
			supported = false;
		}

		if (features.multiViewport == VK_TRUE && gpu->PhysicalDeviceFeatures.multiViewport == VK_FALSE)
		{
			HG_CORE_ERROR("multiViewport feature not supported by physical device.");
			supported = false;
		}

		if (features.samplerAnisotropy == VK_TRUE && gpu->PhysicalDeviceFeatures.samplerAnisotropy == VK_FALSE)
		{
			HG_CORE_ERROR("samplerAnisotropy feature not supported by physical device.");
			supported = false;
		}

		if (features.textureCompressionETC2 == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionETC2 == VK_FALSE)
		{
			HG_CORE_ERROR("textureCompressionETC2 feature not supported by physical device.");
			supported = false;
		}

		if (features.textureCompressionASTC_LDR == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionASTC_LDR == VK_FALSE)
		{
			HG_CORE_ERROR("textureCompressionASTC_LDR feature not supported by physical device.");
			supported = false;
		}

		if (features.textureCompressionBC == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionBC == VK_FALSE)
		{
			HG_CORE_ERROR("textureCompressionBC feature not supported by physical device.");
			supported = false;
		}

		if (features.occlusionQueryPrecise == VK_TRUE && gpu->PhysicalDeviceFeatures.occlusionQueryPrecise == VK_FALSE)
		{
			HG_CORE_ERROR("occlusionQueryPrecise feature not supported by physical device.");
			supported = false;
		}

		if (features.pipelineStatisticsQuery == VK_TRUE && gpu->PhysicalDeviceFeatures.pipelineStatisticsQuery == VK_FALSE)
		{
			HG_CORE_ERROR("pipelineStatisticsQuery feature not supported by physical device.");
			supported = false;
		}

		if (features.vertexPipelineStoresAndAtomics == VK_TRUE && gpu->PhysicalDeviceFeatures.vertexPipelineStoresAndAtomics == VK_FALSE)
		{
			HG_CORE_ERROR("vertexPipelineStoresAndAtomics feature not supported by physical device.");
			supported = false;
		}

		if (features.fragmentStoresAndAtomics == VK_TRUE && gpu->PhysicalDeviceFeatures.fragmentStoresAndAtomics == VK_FALSE)
		{
			HG_CORE_ERROR("fragmentStoresAndAtomics feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderTessellationAndGeometryPointSize == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderTessellationAndGeometryPointSize == VK_FALSE)
		{
			HG_CORE_ERROR("shaderTessellationAndGeometryPointSize feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderImageGatherExtended == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderImageGatherExtended == VK_FALSE)
		{
			HG_CORE_ERROR("shaderImageGatherExtended feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderStorageImageExtendedFormats == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageExtendedFormats == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageImageExtendedFormats feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderStorageImageMultisample == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageMultisample == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageImageMultisample feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderStorageImageReadWithoutFormat == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageReadWithoutFormat == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageImageReadWithoutFormat feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderStorageImageWriteWithoutFormat == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageWriteWithoutFormat == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageImageWriteWithoutFormat feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderUniformBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderUniformBufferArrayDynamicIndexing feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderSampledImageArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderSampledImageArrayDynamicIndexing feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderStorageBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageBufferArrayDynamicIndexing feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderStorageImageArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageImageArrayDynamicIndexing feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderClipDistance == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderClipDistance == VK_FALSE)
		{
			HG_CORE_ERROR("shaderClipDistance feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderCullDistance == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderCullDistance == VK_FALSE)
		{
			HG_CORE_ERROR("shaderCullDistance feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderFloat64 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderFloat64 == VK_FALSE)
		{
			HG_CORE_ERROR("shaderFloat64 feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderInt64 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderInt64 == VK_FALSE)
		{
			HG_CORE_ERROR("shaderInt64 feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderInt16 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderInt16 == VK_FALSE)
		{
			HG_CORE_ERROR("shaderInt16 feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderResourceResidency == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderResourceResidency == VK_FALSE)
		{
			HG_CORE_ERROR("shaderResourceResidency feature not supported by physical device.");
			supported = false;
		}

		if (features.shaderResourceMinLod == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderResourceMinLod == VK_FALSE)
		{
			HG_CORE_ERROR("shaderResourceMinLod feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseBinding == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseBinding == VK_FALSE)
		{
			HG_CORE_ERROR("sparseBinding feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidencyBuffer == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyBuffer == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidencyBuffer feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidencyImage2D == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyImage2D == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidencyImage2D feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidencyImage3D == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyImage3D == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidencyImage3D feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidency2Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency2Samples == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidency2Samples feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidency4Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency4Samples == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidency4Samples feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidency8Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency8Samples == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidency8Samples feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidency16Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency16Samples == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidency16Samples feature not supported by physical device.");
			supported = false;
		}

		if (features.sparseResidencyAliased == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyAliased == VK_FALSE)
		{
			HG_CORE_ERROR("sparseResidencyAliased feature not supported by physical device.");
			supported = false;
		}

		if (features.variableMultisampleRate == VK_TRUE && gpu->PhysicalDeviceFeatures.variableMultisampleRate == VK_FALSE)
		{
			HG_CORE_ERROR("variableMultisampleRate feature not supported by physical device.");
			supported = false;
		}

		if (features.inheritedQueries == VK_TRUE && gpu->PhysicalDeviceFeatures.inheritedQueries == VK_FALSE)
		{
			HG_CORE_ERROR("inheritedQueries feature not supported by physical device.");
			supported = false;
		}

		return supported;
	}

	static bool CheckPhysicalDeviceFeatureSupport11(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceVulkan11Features features)
	{
		bool supported = true;

		if (features.storageBuffer16BitAccess == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.storageBuffer16BitAccess == VK_FALSE)
		{
			HG_CORE_ERROR("storageBuffer16BitAccess feature not supported by physical device");
			supported = false;
		}

		if (features.uniformAndStorageBuffer16BitAccess == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.uniformAndStorageBuffer16BitAccess == VK_FALSE)
		{
			HG_CORE_ERROR("uniformAndStorageBuffer16BitAccess feature not supported by physical device");
			supported = false;
		}

		if (features.storagePushConstant16 == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.storagePushConstant16 == VK_FALSE)
		{
			HG_CORE_ERROR("storagePushConstant16 feature not supported by physical device");
			supported = false;
		}

		if (features.storageInputOutput16 == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.storageInputOutput16 == VK_FALSE)
		{
			HG_CORE_ERROR("storageInputOutput16 feature not supported by physical device");
			supported = false;
		}

		if (features.multiview == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.multiview == VK_FALSE)
		{
			HG_CORE_ERROR("multiview feature not supported by physical device");
			supported = false;
		}

		if (features.multiviewGeometryShader == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.multiviewGeometryShader == VK_FALSE)
		{
			HG_CORE_ERROR("multiviewGeometryShader feature not supported by physical device");
			supported = false;
		}

		if (features.multiviewTessellationShader == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.multiviewTessellationShader == VK_FALSE)
		{
			HG_CORE_ERROR("multiviewTessellationShader feature not supported by physical device");
			supported = false;
		}

		if (features.variablePointersStorageBuffer == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.variablePointersStorageBuffer == VK_FALSE)
		{
			HG_CORE_ERROR("variablePointersStorageBuffer feature not supported by physical device");
			supported = false;
		}

		if (features.variablePointers == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.variablePointers == VK_FALSE)
		{
			HG_CORE_ERROR("variablePointers feature not supported by physical device");
			supported = false;
		}

		if (features.protectedMemory == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.protectedMemory == VK_FALSE)
		{
			HG_CORE_ERROR("protectedMemory feature not supported by physical device");
			supported = false;
		}

		if (features.samplerYcbcrConversion == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.samplerYcbcrConversion == VK_FALSE)
		{
			HG_CORE_ERROR("samplerYcbcrConversion feature not supported by physical device");
			supported = false;
		}

		if (features.shaderDrawParameters == VK_TRUE && gpu->PhysicalDeviceVulkan11Features.shaderDrawParameters == VK_FALSE)
		{
			HG_CORE_ERROR("shaderDrawParameters feature not supported by physical device");
			supported = false;
		}


		return supported;
	}

	static bool CheckPhysicalDeviceFeatureSupport12(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceVulkan12Features features)
	{
		bool supported = true;

		if (features.samplerMirrorClampToEdge == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.samplerMirrorClampToEdge == VK_FALSE)
		{
			HG_CORE_ERROR("samplerMirrorClampToEdge feature not suported by physical device");
			supported = false;
		}

		if (features.drawIndirectCount == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.drawIndirectCount == VK_FALSE)
		{
			HG_CORE_ERROR("drawIndirectCount feature not suported by physical device");
			supported = false;
		}

		if (features.storageBuffer8BitAccess == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.storageBuffer8BitAccess == VK_FALSE)
		{
			HG_CORE_ERROR("storageBuffer8BitAccess feature not suported by physical device");
			supported = false;
		}

		if (features.uniformAndStorageBuffer8BitAccess == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.uniformAndStorageBuffer8BitAccess == VK_FALSE)
		{
			HG_CORE_ERROR("uniformAndStorageBuffer8BitAccess feature not suported by physical device");
			supported = false;
		}

		if (features.storagePushConstant8 == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.storagePushConstant8 == VK_FALSE)
		{
			HG_CORE_ERROR("storagePushConstant8 feature not suported by physical device");
			supported = false;
		}

		if (features.shaderBufferInt64Atomics == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderBufferInt64Atomics == VK_FALSE)
		{
			HG_CORE_ERROR("shaderBufferInt64Atomics feature not suported by physical device");
			supported = false;
		}

		if (features.shaderSharedInt64Atomics == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderSharedInt64Atomics == VK_FALSE)
		{
			HG_CORE_ERROR("shaderSharedInt64Atomics feature not suported by physical device");
			supported = false;
		}

		if (features.shaderFloat16 == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderFloat16 == VK_FALSE)
		{
			HG_CORE_ERROR("shaderFloat16 feature not suported by physical device");
			supported = false;
		}

		if (features.shaderInt8 == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderInt8 == VK_FALSE)
		{
			HG_CORE_ERROR("shaderInt8 feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderInputAttachmentArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderInputAttachmentArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderInputAttachmentArrayDynamicIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderUniformTexelBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderUniformTexelBufferArrayDynamicIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderStorageTexelBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageTexelBufferArrayDynamicIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderUniformBufferArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderUniformBufferArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderUniformBufferArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderSampledImageArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderSampledImageArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderStorageBufferArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageBufferArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderStorageImageArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderStorageImageArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageImageArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderInputAttachmentArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderInputAttachmentArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderInputAttachmentArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderUniformTexelBufferArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderUniformTexelBufferArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.shaderStorageTexelBufferArrayNonUniformIndexing == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing == VK_FALSE)
		{
			HG_CORE_ERROR("shaderStorageTexelBufferArrayNonUniformIndexing feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingUniformBufferUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingUniformBufferUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingUniformBufferUpdateAfterBind feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingSampledImageUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingSampledImageUpdateAfterBind feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingStorageImageUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingStorageImageUpdateAfterBind feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingStorageBufferUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingStorageBufferUpdateAfterBind feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingUniformTexelBufferUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingUniformTexelBufferUpdateAfterBind feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingStorageTexelBufferUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingStorageTexelBufferUpdateAfterBind feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingUpdateUnusedWhilePending == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingUpdateUnusedWhilePending == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingUpdateUnusedWhilePending feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingPartiallyBound == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingPartiallyBound == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingPartiallyBound feature not suported by physical device");
			supported = false;
		}

		if (features.descriptorBindingVariableDescriptorCount == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.descriptorBindingVariableDescriptorCount == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingVariableDescriptorCount feature not suported by physical device");
			supported = false;
		}

		if (features.runtimeDescriptorArray == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.runtimeDescriptorArray == VK_FALSE)
		{
			HG_CORE_ERROR("runtimeDescriptorArray feature not suported by physical device");
			supported = false;
		}

		if (features.samplerFilterMinmax == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.samplerFilterMinmax == VK_FALSE)
		{
			HG_CORE_ERROR("samplerFilterMinmax feature not suported by physical device");
			supported = false;
		}

		if (features.scalarBlockLayout == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.scalarBlockLayout == VK_FALSE)
		{
			HG_CORE_ERROR("scalarBlockLayout feature not suported by physical device");
			supported = false;
		}

		if (features.imagelessFramebuffer == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.imagelessFramebuffer == VK_FALSE)
		{
			HG_CORE_ERROR("imagelessFramebuffer feature not suported by physical device");
			supported = false;
		}

		if (features.uniformBufferStandardLayout == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.uniformBufferStandardLayout == VK_FALSE)
		{
			HG_CORE_ERROR("uniformBufferStandardLayout feature not suported by physical device");
			supported = false;
		}

		if (features.shaderSubgroupExtendedTypes == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderSubgroupExtendedTypes == VK_FALSE)
		{
			HG_CORE_ERROR("shaderSubgroupExtendedTypes feature not suported by physical device");
			supported = false;
		}

		if (features.separateDepthStencilLayouts == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.separateDepthStencilLayouts == VK_FALSE)
		{
			HG_CORE_ERROR("separateDepthStencilLayouts feature not suported by physical device");
			supported = false;
		}

		if (features.hostQueryReset == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.hostQueryReset == VK_FALSE)
		{
			HG_CORE_ERROR("hostQueryReset feature not suported by physical device");
			supported = false;
		}

		if (features.timelineSemaphore == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.timelineSemaphore == VK_FALSE)
		{
			HG_CORE_ERROR("timelineSemaphore feature not suported by physical device");
			supported = false;
		}

		if (features.bufferDeviceAddress == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.bufferDeviceAddress == VK_FALSE)
		{
			HG_CORE_ERROR("bufferDeviceAddress feature not suported by physical device");
			supported = false;
		}

		if (features.bufferDeviceAddressCaptureReplay == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.bufferDeviceAddressCaptureReplay == VK_FALSE)
		{
			HG_CORE_ERROR("bufferDeviceAddressCaptureReplay feature not suported by physical device");
			supported = false;
		}

		if (features.bufferDeviceAddressMultiDevice == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.bufferDeviceAddressMultiDevice == VK_FALSE)
		{
			HG_CORE_ERROR("bufferDeviceAddressMultiDevice feature not suported by physical device");
			supported = false;
		}

		if (features.vulkanMemoryModel == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.vulkanMemoryModel == VK_FALSE)
		{
			HG_CORE_ERROR("vulkanMemoryModel feature not suported by physical device");
			supported = false;
		}

		if (features.vulkanMemoryModelDeviceScope == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.vulkanMemoryModelDeviceScope == VK_FALSE)
		{
			HG_CORE_ERROR("vulkanMemoryModelDeviceScope feature not suported by physical device");
			supported = false;
		}

		if (features.vulkanMemoryModelAvailabilityVisibilityChains == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.vulkanMemoryModelAvailabilityVisibilityChains == VK_FALSE)
		{
			HG_CORE_ERROR("vulkanMemoryModelAvailabilityVisibilityChains feature not suported by physical device");
			supported = false;
		}

		if (features.shaderOutputViewportIndex == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderOutputViewportIndex == VK_FALSE)
		{
			HG_CORE_ERROR("shaderOutputViewportIndex feature not suported by physical device");
			supported = false;
		}

		if (features.shaderOutputLayer == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.shaderOutputLayer == VK_FALSE)
		{
			HG_CORE_ERROR("shaderOutputLayer feature not suported by physical device");
			supported = false;
		}

		if (features.subgroupBroadcastDynamicId == VK_TRUE && gpu->PhysicalDeviceVulkan12Features.subgroupBroadcastDynamicId == VK_FALSE)
		{
			HG_CORE_ERROR("subgroupBroadcastDynamicId feature not suported by physical device");
			supported = false;
		}


		return supported;
	}

	static bool CheckPhysicalDeviceFeatureSupport13(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceVulkan13Features  features)
	{
		bool supported = true;

		if (features.robustImageAccess == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.robustImageAccess == VK_FALSE)
		{
			HG_CORE_ERROR("robustImageAccess feature is not supported by physical device");
			supported = false;
		}

		if (features.inlineUniformBlock == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.inlineUniformBlock == VK_FALSE)
		{
			HG_CORE_ERROR("inlineUniformBlock feature is not supported by physical device");
			supported = false;
		}

		if (features.descriptorBindingInlineUniformBlockUpdateAfterBind == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.descriptorBindingInlineUniformBlockUpdateAfterBind == VK_FALSE)
		{
			HG_CORE_ERROR("descriptorBindingInlineUniformBlockUpdateAfterBind feature is not supported by physical device");
			supported = false;
		}

		if (features.pipelineCreationCacheControl == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.pipelineCreationCacheControl == VK_FALSE)
		{
			HG_CORE_ERROR("pipelineCreationCacheControl feature is not supported by physical device");
			supported = false;
		}

		if (features.privateData == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.privateData == VK_FALSE)
		{
			HG_CORE_ERROR("privateData feature is not supported by physical device");
			supported = false;
		}

		if (features.shaderDemoteToHelperInvocation == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.shaderDemoteToHelperInvocation == VK_FALSE)
		{
			HG_CORE_ERROR("shaderDemoteToHelperInvocation feature is not supported by physical device");
			supported = false;
		}

		if (features.shaderTerminateInvocation == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.shaderTerminateInvocation == VK_FALSE)
		{
			HG_CORE_ERROR("shaderTerminateInvocation feature is not supported by physical device");
			supported = false;
		}

		if (features.subgroupSizeControl == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.subgroupSizeControl == VK_FALSE)
		{
			HG_CORE_ERROR("subgroupSizeControl feature is not supported by physical device");
			supported = false;
		}

		if (features.computeFullSubgroups == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.computeFullSubgroups == VK_FALSE)
		{
			HG_CORE_ERROR("computeFullSubgroups feature is not supported by physical device");
			supported = false;
		}

		if (features.synchronization2 == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.synchronization2 == VK_FALSE)
		{
			HG_CORE_ERROR("synchronization2 feature is not supported by physical device");
			supported = false;
		}

		if (features.textureCompressionASTC_HDR == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.textureCompressionASTC_HDR == VK_FALSE)
		{
			HG_CORE_ERROR("textureCompressionASTC_HDR feature is not supported by physical device");
			supported = false;
		}

		if (features.shaderZeroInitializeWorkgroupMemory == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.shaderZeroInitializeWorkgroupMemory == VK_FALSE)
		{
			HG_CORE_ERROR("shaderZeroInitializeWorkgroupMemory feature is not supported by physical device");
			supported = false;
		}

		if (features.dynamicRendering == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.dynamicRendering == VK_FALSE)
		{
			HG_CORE_ERROR("dynamicRendering feature is not supported by physical device");
			supported = false;
		}

		if (features.shaderIntegerDotProduct == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.shaderIntegerDotProduct == VK_FALSE)
		{
			HG_CORE_ERROR("shaderIntegerDotProduct feature is not supported by physical device");
			supported = false;
		}

		if (features.maintenance4 == VK_TRUE && gpu->PhysicalDeviceVulkan13Features.maintenance4 == VK_FALSE)
		{
			HG_CORE_ERROR("maintenance4 feature is not supported by physical device");
			supported = false;
		}


		return supported;
	}

}
