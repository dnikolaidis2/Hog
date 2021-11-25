#include "vkcpch.h"
#include "GraphicsContext.h"

#include "vk_mem_alloc.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Utils/RendererUtils.h"

namespace VulkanCore {

	static bool CheckPhysicalDeviceFeatureSupport(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceFeatures features)
	{
		bool supported = true;
		if (features.robustBufferAccess == VK_TRUE && gpu->PhysicalDeviceFeatures.robustBufferAccess != features.robustBufferAccess)
		{
			VKC_CORE_ERROR("robustBufferAccess feature not supported by physical device.");
			supported = false;
		}

	    if (features.fullDrawIndexUint32 == VK_TRUE && gpu->PhysicalDeviceFeatures.fullDrawIndexUint32 != features.fullDrawIndexUint32)
	    {
	    	VKC_CORE_ERROR("fullDrawIndexUint32 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.imageCubeArray == VK_TRUE && gpu->PhysicalDeviceFeatures.imageCubeArray != features.imageCubeArray)
	    {
	    	VKC_CORE_ERROR("imageCubeArray feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.independentBlend == VK_TRUE && gpu->PhysicalDeviceFeatures.independentBlend != features.independentBlend)
	    {
	    	VKC_CORE_ERROR("independentBlend feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.geometryShader == VK_TRUE && gpu->PhysicalDeviceFeatures.geometryShader != features.geometryShader)
	    {
	    	VKC_CORE_ERROR("geometryShader feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.tessellationShader == VK_TRUE && gpu->PhysicalDeviceFeatures.tessellationShader != features.tessellationShader)
	    {
	    	VKC_CORE_ERROR("tessellationShader feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sampleRateShading == VK_TRUE && gpu->PhysicalDeviceFeatures.sampleRateShading != features.sampleRateShading)
	    {
	    	VKC_CORE_ERROR("sampleRateShading feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.dualSrcBlend == VK_TRUE && gpu->PhysicalDeviceFeatures.dualSrcBlend != features.dualSrcBlend)
	    {
	    	VKC_CORE_ERROR("dualSrcBlend feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.logicOp == VK_TRUE && gpu->PhysicalDeviceFeatures.logicOp != features.logicOp)
	    {
	    	VKC_CORE_ERROR("logicOp feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.multiDrawIndirect == VK_TRUE && gpu->PhysicalDeviceFeatures.multiDrawIndirect != features.multiDrawIndirect)
	    {
	    	VKC_CORE_ERROR("multiDrawIndirect feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.drawIndirectFirstInstance == VK_TRUE && gpu->PhysicalDeviceFeatures.drawIndirectFirstInstance != features.drawIndirectFirstInstance)
	    {
	    	VKC_CORE_ERROR("drawIndirectFirstInstance feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.depthClamp == VK_TRUE && gpu->PhysicalDeviceFeatures.depthClamp != features.depthClamp)
	    {
	    	VKC_CORE_ERROR("depthClamp feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.depthBiasClamp == VK_TRUE && gpu->PhysicalDeviceFeatures.depthBiasClamp != features.depthBiasClamp)
	    {
	    	VKC_CORE_ERROR("depthBiasClamp feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.fillModeNonSolid == VK_TRUE && gpu->PhysicalDeviceFeatures.fillModeNonSolid != features.fillModeNonSolid)
	    {
	    	VKC_CORE_ERROR("fillModeNonSolid feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.depthBounds == VK_TRUE && gpu->PhysicalDeviceFeatures.depthBounds != features.depthBounds)
	    {
	    	VKC_CORE_ERROR("depthBounds feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.wideLines == VK_TRUE && gpu->PhysicalDeviceFeatures.wideLines != features.wideLines)
	    {
	    	VKC_CORE_ERROR("wideLines feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.largePoints == VK_TRUE && gpu->PhysicalDeviceFeatures.largePoints != features.largePoints)
	    {
	    	VKC_CORE_ERROR("largePoints feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.alphaToOne == VK_TRUE && gpu->PhysicalDeviceFeatures.alphaToOne != features.alphaToOne)
	    {
	    	VKC_CORE_ERROR("alphaToOne feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.multiViewport == VK_TRUE && gpu->PhysicalDeviceFeatures.multiViewport != features.multiViewport)
	    {
	    	VKC_CORE_ERROR("multiViewport feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.samplerAnisotropy == VK_TRUE && gpu->PhysicalDeviceFeatures.samplerAnisotropy != features.samplerAnisotropy)
	    {
	    	VKC_CORE_ERROR("samplerAnisotropy feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.textureCompressionETC2 == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionETC2 != features.textureCompressionETC2)
	    {
	    	VKC_CORE_ERROR("textureCompressionETC2 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.textureCompressionASTC_LDR == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionASTC_LDR != features.textureCompressionASTC_LDR)
	    {
	    	VKC_CORE_ERROR("textureCompressionASTC_LDR feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.textureCompressionBC == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionBC != features.textureCompressionBC)
	    {
	    	VKC_CORE_ERROR("textureCompressionBC feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.occlusionQueryPrecise == VK_TRUE && gpu->PhysicalDeviceFeatures.occlusionQueryPrecise != features.occlusionQueryPrecise)
	    {
	    	VKC_CORE_ERROR("occlusionQueryPrecise feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.pipelineStatisticsQuery == VK_TRUE && gpu->PhysicalDeviceFeatures.pipelineStatisticsQuery != features.pipelineStatisticsQuery)
	    {
	    	VKC_CORE_ERROR("pipelineStatisticsQuery feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.vertexPipelineStoresAndAtomics == VK_TRUE && gpu->PhysicalDeviceFeatures.vertexPipelineStoresAndAtomics != features.vertexPipelineStoresAndAtomics)
	    {
	    	VKC_CORE_ERROR("vertexPipelineStoresAndAtomics feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.fragmentStoresAndAtomics == VK_TRUE && gpu->PhysicalDeviceFeatures.fragmentStoresAndAtomics != features.fragmentStoresAndAtomics)
	    {
	    	VKC_CORE_ERROR("fragmentStoresAndAtomics feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderTessellationAndGeometryPointSize == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderTessellationAndGeometryPointSize != features.shaderTessellationAndGeometryPointSize)
	    {
	    	VKC_CORE_ERROR("shaderTessellationAndGeometryPointSize feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderImageGatherExtended == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderImageGatherExtended != features.shaderImageGatherExtended)
	    {
	    	VKC_CORE_ERROR("shaderImageGatherExtended feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageExtendedFormats == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageExtendedFormats != features.shaderStorageImageExtendedFormats)
	    {
	    	VKC_CORE_ERROR("shaderStorageImageExtendedFormats feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageMultisample == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageMultisample != features.shaderStorageImageMultisample)
	    {
	    	VKC_CORE_ERROR("shaderStorageImageMultisample feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageReadWithoutFormat == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageReadWithoutFormat != features.shaderStorageImageReadWithoutFormat)
	    {
	    	VKC_CORE_ERROR("shaderStorageImageReadWithoutFormat feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageWriteWithoutFormat == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageWriteWithoutFormat != features.shaderStorageImageWriteWithoutFormat)
	    {
	    	VKC_CORE_ERROR("shaderStorageImageWriteWithoutFormat feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderUniformBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing != features.shaderUniformBufferArrayDynamicIndexing)
	    {
	    	VKC_CORE_ERROR("shaderUniformBufferArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderSampledImageArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing != features.shaderSampledImageArrayDynamicIndexing)
	    {
	    	VKC_CORE_ERROR("shaderSampledImageArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing != features.shaderStorageBufferArrayDynamicIndexing)
	    {
	    	VKC_CORE_ERROR("shaderStorageBufferArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing != features.shaderStorageImageArrayDynamicIndexing)
	    {
	    	VKC_CORE_ERROR("shaderStorageImageArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderClipDistance == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderClipDistance != features.shaderClipDistance)
	    {
	    	VKC_CORE_ERROR("shaderClipDistance feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderCullDistance == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderCullDistance != features.shaderCullDistance)
	    {
	    	VKC_CORE_ERROR("shaderCullDistance feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderFloat64 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderFloat64 != features.shaderFloat64)
	    {
	    	VKC_CORE_ERROR("shaderFloat64 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderInt64 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderInt64 != features.shaderInt64)
	    {
	    	VKC_CORE_ERROR("shaderInt64 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderInt16 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderInt16 != features.shaderInt16)
	    {
	    	VKC_CORE_ERROR("shaderInt16 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderResourceResidency == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderResourceResidency != features.shaderResourceResidency)
	    {
	    	VKC_CORE_ERROR("shaderResourceResidency feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderResourceMinLod == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderResourceMinLod != features.shaderResourceMinLod)
	    {
	    	VKC_CORE_ERROR("shaderResourceMinLod feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseBinding == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseBinding != features.sparseBinding)
	    {
	    	VKC_CORE_ERROR("sparseBinding feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyBuffer == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyBuffer != features.sparseResidencyBuffer)
	    {
	    	VKC_CORE_ERROR("sparseResidencyBuffer feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyImage2D == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyImage2D != features.sparseResidencyImage2D)
	    {
	    	VKC_CORE_ERROR("sparseResidencyImage2D feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyImage3D == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyImage3D != features.sparseResidencyImage3D)
	    {
	    	VKC_CORE_ERROR("sparseResidencyImage3D feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency2Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency2Samples != features.sparseResidency2Samples)
	    {
	    	VKC_CORE_ERROR("sparseResidency2Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency4Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency4Samples != features.sparseResidency4Samples)
	    {
	    	VKC_CORE_ERROR("sparseResidency4Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency8Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency8Samples != features.sparseResidency8Samples)
	    {
	    	VKC_CORE_ERROR("sparseResidency8Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency16Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency16Samples != features.sparseResidency16Samples)
	    {
	    	VKC_CORE_ERROR("sparseResidency16Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyAliased == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyAliased != features.sparseResidencyAliased)
	    {
	    	VKC_CORE_ERROR("sparseResidencyAliased feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.variableMultisampleRate == VK_TRUE && gpu->PhysicalDeviceFeatures.variableMultisampleRate != features.variableMultisampleRate)
	    {
	    	VKC_CORE_ERROR("variableMultisampleRate feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.inheritedQueries == VK_TRUE && gpu->PhysicalDeviceFeatures.inheritedQueries != features.inheritedQueries)
	    {
	    	VKC_CORE_ERROR("inheritedQueries feature not supported by physical device.");
	    	supported = false;
	    }

	    return supported;
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

	static VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats)
	{
		VkSurfaceFormatKHR result;

		// If Vulkan returned an unknown format, then just force what we want.
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) 
		{
			result.format = VK_FORMAT_B8G8R8A8_UNORM;
			result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			return result;
		}

		// Favor 32 bit rgba and srgb nonlinear colorspace
		for (int i = 0; i < formats.size(); ++i) 
		{
			VkSurfaceFormatKHR& fmt = formats[i];
			if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
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
		VKC_PROFILE_FUNCTION();
		CreateInstance();
		SetupDebugMessenger();
		Application::Get().GetWindow().CreateSurface(Instance, nullptr, &(Surface));
		EnumeratePhysicalDevices();
		SelectPhysicalDevice();
		CreateLogicalDeviceAndQueues();
		InitializeAllocator();
		CreateSemaphores();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSwapChain();
		CreateRenderPass();
		CreateFrameBuffers();

		Initialized = true;
	}

	void GraphicsContext::DeinitializeImpl()
	{
		vkFreeCommandBuffers(Device, CommandPool, (uint32_t)(CommandBuffers.size()), CommandBuffers.data());

		for (auto framebuffer : FrameBuffers) {
			vkDestroyFramebuffer(Device, framebuffer, nullptr);
		}

		vkDestroyRenderPass(Device, RenderPass, nullptr);

		for (auto image : SwapchainImages)
		{
			vkDestroyImageView(Device, image.View, nullptr);
		}

		vkDestroySwapchainKHR(Device, Swapchain, nullptr);

		for (auto fence : CommandBufferFences)
		{
			vkDestroyFence(Device, fence, nullptr);
		}

		vkDestroyCommandPool(Device, CommandPool, nullptr);

		for (int i = 0; i < FrameCount; ++i)
		{
			vkDestroySemaphore(Device, AcquireSemaphores[i], nullptr);
			vkDestroySemaphore(Device, RenderCompleteSemaphores[i], nullptr);
		}

		vmaDestroyAllocator(Allocator);

		vkDestroyDevice(Device, nullptr);
		vkDestroySurfaceKHR(Instance, Surface, nullptr);

		if (EnableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
		}

		DestroyInstance();

		Initialized = false;
	}

	void GraphicsContext::RecreateSwapChainImpl()
	{
		CleanupSwapChain();

		CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPU->Device, Surface, &GPU->SurfaceCapabilities));
		CreateSwapChain();
		CreateRenderPass();
		CreateFrameBuffers();
		CreateCommandBuffers();
	}

	void GraphicsContext::CreateInstance()
	{
		VKC_PROFILE_FUNCTION();

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &(ApplicationInfo);

		if (EnableValidationLayers) {
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

			VKC_ASSERT(exists, "Extension not supported");
		}

		if (EnableValidationLayers)
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

				VKC_ASSERT(exists, "Extension not supported");
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
		VKC_PROFILE_FUNCTION();
		if (!EnableValidationLayers) return;
		
		CheckVkResult(CreateDebugUtilsMessengerEXT(Instance, &DebugMessengerCreateInfo, nullptr, &DebugMessenger));
	}

	void GraphicsContext::EnumeratePhysicalDevices()
	{
		VKC_PROFILE_FUNCTION();
		// CheckVkResult and VKC_ASSERT are simply macros for checking return values,
		// and then taking action if necessary.
	
		// First just get the number of devices.
		uint32_t numDevices = 0;
		CheckVkResult(vkEnumeratePhysicalDevices(Instance, &numDevices, nullptr));
		VKC_ASSERT(numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices.")

		std::vector<VkPhysicalDevice> devices(numDevices);

		// Now get the actual devices
		CheckVkResult(vkEnumeratePhysicalDevices(Instance, &numDevices, devices.data()));
		VKC_ASSERT(numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices.")

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
				VKC_ASSERT(numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues.");

				gpu.QueueFamilyProperties.resize(numQueues);
				vkGetPhysicalDeviceQueueFamilyProperties(gpu.Device, &numQueues, gpu.QueueFamilyProperties.data());
				VKC_ASSERT(numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues.");
			}

			{
				// Next let's get the extensions supported by the device.
				uint32_t numExtension;
				CheckVkResult(vkEnumerateDeviceExtensionProperties(gpu.Device, nullptr, &numExtension, nullptr));
				VKC_ASSERT(numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions.");

				gpu.ExtensionProperties.resize(numExtension);
				CheckVkResult(vkEnumerateDeviceExtensionProperties(gpu.Device, nullptr, &numExtension, gpu.ExtensionProperties.data()));
				VKC_ASSERT(numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions.");
			}

			// Surface capabilities basically describes what kind of image you can render to the user.
			// Look up VkSurfaceCapabilitiesKHR in the Vulkan documentation.
			CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.Device, Surface, &gpu.SurfaceCapabilities));

			{
				// Get the supported surface formats.  This includes image format and color space.
				// A common format is VK_FORMAT_R8G8B8A8_UNORM which is 8 bits for red, green, blue, alpha making for 32 total
				uint32_t numFormats;
				CheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Device, Surface, &numFormats, nullptr));
				VKC_ASSERT(numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats.");

				gpu.SurfaceFormats.resize(numFormats);
				CheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.Device, Surface, &numFormats, gpu.SurfaceFormats.data()));
				VKC_ASSERT(numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats.");
			}

			{
				// Vulkan supports multiple presentation modes, and I'll linkn to some good documentation on that in just a bit.
				uint32_t numPresentModes;
				CheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Device, Surface, &numPresentModes, nullptr));
				VKC_ASSERT(numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes.");

				gpu.PresentModes.resize(numPresentModes);
				CheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.Device, Surface, &numPresentModes, gpu.PresentModes.data()));
				VKC_ASSERT(numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes.");
			}

			{
				// Get physical device features to check with features required by application
				vkGetPhysicalDeviceFeatures(gpu.Device, &gpu.PhysicalDeviceFeatures);
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
		VKC_PROFILE_FUNCTION();
		// Let's pick a GPU!
		for (uint32_t i = 0; i < GPUs.size(); i++)
		{
			GPUInfo* gpu = &(GPUs[i]);
			// This is again related to queues.  Don't worry I'll get there soon.
			int graphicsIdx = -1;
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

			// Did we find a device supporting both graphics and present.
			if (graphicsIdx >= 0 && presentIdx >= 0) 
			{
				GraphicsFamilyIndex = (uint32_t)graphicsIdx;
				PresentFamilyIndex = (uint32_t)presentIdx;
				PhysicalDevice = gpu->Device;
				GPU = gpu;
				return;
			}
		}

		// If we can't render or present, just bail.
		// DIAF
		VKC_CORE_ERROR("Could not find a physical device which fits our desired profile");
		VKC_CORE_ASSERT(false)
	}

	void GraphicsContext::CreateLogicalDeviceAndQueues()
	{
		VKC_PROFILE_FUNCTION();
		// Add each family index to a list.
		// Don't do duplicates
		std::vector<uint32_t> uniqueIdx;
		uniqueIdx.push_back(GraphicsFamilyIndex);
		if (std::find(uniqueIdx.begin(), uniqueIdx.end(), PresentFamilyIndex) == uniqueIdx.end()) {
			uniqueIdx.push_back(PresentFamilyIndex);
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
		info.queueCreateInfoCount = (uint32_t)devqInfo.size();
		info.pQueueCreateInfos = devqInfo.data();
		info.pEnabledFeatures = &DeviceFeatures;
		info.enabledExtensionCount = (uint32_t)DeviceExtensions.size();
		info.ppEnabledExtensionNames = DeviceExtensions.data();

		// If validation layers are enabled supply them here.
		if (EnableValidationLayers) {
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
	}

	void GraphicsContext::InitializeAllocator()
	{
		VKC_PROFILE_FUNCTION();
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;

		vmaCreateAllocator(&allocatorInfo, &Allocator);
	}

	void GraphicsContext::CreateSemaphores()
	{
		VKC_PROFILE_FUNCTION();
		AcquireSemaphores.resize(FrameCount);
		RenderCompleteSemaphores.resize(FrameCount);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (int i = 0; i < FrameCount; ++i) {
			CheckVkResult(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &(AcquireSemaphores[i])));
			CheckVkResult(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &(RenderCompleteSemaphores[i])));
		}
	}

	void GraphicsContext::CreateCommandPool()
	{
		VKC_PROFILE_FUNCTION();
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

		CheckVkResult(vkCreateCommandPool(Device, &commandPoolCreateInfo, nullptr, &CommandPool));
	}

	void GraphicsContext::CreateCommandBuffers()
	{
		VKC_PROFILE_FUNCTION();
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		// Don't worry about this
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		// The command pool we created above
		commandBufferAllocateInfo.commandPool = CommandPool;

		// We'll have two command buffers.  One will be in flight
		// while the other is being built.
		commandBufferAllocateInfo.commandBufferCount = FrameCount;

		CommandBuffers.resize(FrameCount);

		// You can allocate multiple command buffers at once.
		CheckVkResult(vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, CommandBuffers.data()));

		// We create fences that we can use to wait for a 
		// given command buffer to be done on the GPU.
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		CommandBufferFences.resize(FrameCount);
		for (int i = 0; i < FrameCount; ++i)
		{
			CheckVkResult(vkCreateFence(Device, &fenceCreateInfo, nullptr, &CommandBufferFences[i]));
		}
	}

	void GraphicsContext::CreateSwapChain()
	{
		VKC_PROFILE_FUNCTION();
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
		info.minImageCount = FrameCount;

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
		std::vector<VkImage> swapchainImages(FrameCount);
		CheckVkResult(vkGetSwapchainImagesKHR(Device, Swapchain, &numImages, nullptr));
		VKC_ASSERT(numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count.")

		// Second call uses numImages
		CheckVkResult(vkGetSwapchainImagesKHR(Device, Swapchain, &numImages, swapchainImages.data()));
		VKC_ASSERT(numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count.");

		SwapchainImages.resize(FrameCount);

		// New concept - Image Views
		// Much like the logical device is an interface to the physical device,
		// image views are interfaces to actual images.  Think of it as this.
		// The image exists outside of you.  But the view is your personal view 
		// ( how you perceive ) the image.
		for (int i = 0; i < FrameCount; ++i) {
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			// Just plug it in
			imageViewCreateInfo.image = swapchainImages[i];

			// These are 2D images
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			// The selected format
			imageViewCreateInfo.format = SwapchainFormat;

			// We don't need to swizzle ( swap around ) any of the 
			// color channels
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

			// There are only 4x aspect bits.  And most people will only use 3x.
			// These determine what is affected by your image operations.
			// VK_IMAGE_ASPECT_COLOR_BIT
			// VK_IMAGE_ASPECT_DEPTH_BIT
			// VK_IMAGE_ASPECT_STENCIL_BIT
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			// For beginners - a base mip level of zero is par for the course.
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;

			// Level count is the # of images visible down the mip chain.
			// So basically just 1...
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			// We don't have multiple layers to these images.
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.flags = 0;

			// Create the view
			VkImageView imageView;
			CheckVkResult(vkCreateImageView(Device, &imageViewCreateInfo, nullptr, &imageView));

			// Now store this off in an idImage so we can take advantage
			// of that class's API
			Image image(
				swapchainImages[i],
				imageView,
				SwapchainFormat,
				SwapchainExtent
			);
			image.IsSwapChainImage = true;
			SwapchainImages[i] = image;
		}
	}

	VkFormat GraphicsContext::ChooseSupportedFormat(VkFormat* formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		VKC_PROFILE_FUNCTION();
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

		VKC_CORE_ERROR("Failed to find a supported format.");

		return VK_FORMAT_UNDEFINED;
	}

	void GraphicsContext::CreateRenderTargets()
	{
		VKC_PROFILE_FUNCTION();
		// Select Depth Format, prefer as high a precision as we can get.
		{
			VkFormat formats[] = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT
			};

			// Make sure to check it supports optimal tiling and is a depth/stencil format.
			DepthFormat = ChooseSupportedFormat(
					formats, 2,
					VK_IMAGE_TILING_OPTIMAL,
					VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		}

		// idTech4.5 does have an independent idea of a depth attachment
		// So now that the context contains the selected format we can simply
		// create the internal one.

		DepthImage.Type = ImageType::Depth;
		DepthImage.Width = Application::Get().GetWindow().GetFrameBufferWidth();
		DepthImage.Height = Application::Get().GetWindow().GetFrameBufferHeight();
		DepthImage.LevelCount = 1;
	}

	void GraphicsContext::CreateRenderPass()
	{
		VKC_PROFILE_FUNCTION();
		std::vector<VkAttachmentDescription> attachments;

		// VkNeo uses a single renderpass, so I just create it on startup.
		// Attachments act as slots in which to insert images.

		// For the color attachment, we'll simply be using the swapchain images.
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = SwapchainFormat;
		// Sample count goes from 1 - 64
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// I don't care what you do with the image memory when you load it for use.
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// Just store the image when you go to store it.
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// I don't care what the initial layout of the image is.
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// It better be ready to present to the user when we're done with the renderpass.
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments.push_back(colorAttachment);

		// For the depth attachment, we'll be using the _viewDepth we just created.
		// VkAttachmentDescription depthAttachment = {};
		// depthAttachment.format = DepthFormat;
		// depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		// attachments.push_back(depthAttachment);

		// Now we enumerate the attachments for a subpass.  We have to have at least one subpass.
		VkAttachmentReference colorRef = {};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// VkAttachmentReference depthRef = {};
		// depthRef.attachment = 1;
		// depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Basically is this graphics or compute
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;
		// subpass.pDepthStencilAttachment = &depthRef;

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 0;

		CheckVkResult(vkCreateRenderPass(Device, &renderPassCreateInfo, nullptr, &RenderPass));
	}

	void GraphicsContext::CreateFrameBuffers()
	{
		VKC_PROFILE_FUNCTION();
		FrameBuffers.resize(FrameCount);

		VkImageView attachments[1];

		// Depth attachment is the same
		// We never show the depth buffer, so we only ever need one.
		// idImage* depthImg = globalImages->GetImage("_viewDepth");
		// if (depthImg == NULL) {
		// 	idLib::FatalError("CreateFrameBuffers: No _viewDepth image.");
		// }
		//
		// attachments[1] = depthImg->GetView();

		// VkFrameBuffer is what maps attachments to a renderpass.  That's really all it is.
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// The renderpass we just created.
		frameBufferCreateInfo.renderPass = RenderPass;
		// The color and depth attachments
		frameBufferCreateInfo.attachmentCount = 1;
		frameBufferCreateInfo.pAttachments = attachments;
		// Current render size
		frameBufferCreateInfo.width = SwapchainExtent.width;
		frameBufferCreateInfo.height = SwapchainExtent.height;
		frameBufferCreateInfo.layers = 1;

		// Because we're double buffering, we need to create the same number of framebuffers.
		// The main difference again is that both of them use the same depth image view.
		for (int i = 0; i < FrameCount; ++i) {
			attachments[0] = SwapchainImages[i].View;
			CheckVkResult(vkCreateFramebuffer(Device, &frameBufferCreateInfo, NULL, &FrameBuffers[i]));
		}
	}

	void GraphicsContext::CleanupSwapChain()
	{
		VKC_PROFILE_FUNCTION();
		for (size_t i = 0; i < FrameBuffers.size(); i++) {
			vkDestroyFramebuffer(Device, FrameBuffers[i], nullptr);
		}

		vkFreeCommandBuffers(Device, CommandPool, (uint32_t)(CommandBuffers.size()), CommandBuffers.data());
		for (auto fence : CommandBufferFences)
		{
			vkDestroyFence(Device, fence, nullptr);
		}

		vkDestroyRenderPass(Device, RenderPass, nullptr);

		for (size_t i = 0; i < SwapchainImages.size(); i++) {
			vkDestroyImageView(Device, SwapchainImages[i].View, nullptr);
		}

		// vkDestroySwapchainKHR(Device, Swapchain, nullptr);
	}
}
