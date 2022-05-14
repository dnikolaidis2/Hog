#include "hgpch.h"
#include "GraphicsContext.h"

#include "vk_mem_alloc.h"
#include "Hog/Core/CVars.h"
#include "Hog/Core/Application.h"
#include "Hog/Utils/RendererUtils.h"

AutoCVar_Int CVar_MSAA("renderer.enableMSAA", "Enables MSAA for renderer", 0, CVarFlags::EditReadOnly);
AutoCVar_Int CVar_ValidationLayers("renderer.enableValidationLayers", "Enables Vulkan validation layers", 1, CVarFlags::EditReadOnly);

namespace Hog {

	static bool CheckPhysicalDeviceFeatureSupport(GraphicsContext::GPUInfo* gpu, VkPhysicalDeviceFeatures features)
	{
		bool supported = true;
		if (features.robustBufferAccess == VK_TRUE && gpu->PhysicalDeviceFeatures.robustBufferAccess != features.robustBufferAccess)
		{
			HG_CORE_ERROR("robustBufferAccess feature not supported by physical device.");
			supported = false;
		}

	    if (features.fullDrawIndexUint32 == VK_TRUE && gpu->PhysicalDeviceFeatures.fullDrawIndexUint32 != features.fullDrawIndexUint32)
	    {
	    	HG_CORE_ERROR("fullDrawIndexUint32 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.imageCubeArray == VK_TRUE && gpu->PhysicalDeviceFeatures.imageCubeArray != features.imageCubeArray)
	    {
	    	HG_CORE_ERROR("imageCubeArray feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.independentBlend == VK_TRUE && gpu->PhysicalDeviceFeatures.independentBlend != features.independentBlend)
	    {
	    	HG_CORE_ERROR("independentBlend feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.geometryShader == VK_TRUE && gpu->PhysicalDeviceFeatures.geometryShader != features.geometryShader)
	    {
	    	HG_CORE_ERROR("geometryShader feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.tessellationShader == VK_TRUE && gpu->PhysicalDeviceFeatures.tessellationShader != features.tessellationShader)
	    {
	    	HG_CORE_ERROR("tessellationShader feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sampleRateShading == VK_TRUE && gpu->PhysicalDeviceFeatures.sampleRateShading != features.sampleRateShading)
	    {
	    	HG_CORE_ERROR("sampleRateShading feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.dualSrcBlend == VK_TRUE && gpu->PhysicalDeviceFeatures.dualSrcBlend != features.dualSrcBlend)
	    {
	    	HG_CORE_ERROR("dualSrcBlend feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.logicOp == VK_TRUE && gpu->PhysicalDeviceFeatures.logicOp != features.logicOp)
	    {
	    	HG_CORE_ERROR("logicOp feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.multiDrawIndirect == VK_TRUE && gpu->PhysicalDeviceFeatures.multiDrawIndirect != features.multiDrawIndirect)
	    {
	    	HG_CORE_ERROR("multiDrawIndirect feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.drawIndirectFirstInstance == VK_TRUE && gpu->PhysicalDeviceFeatures.drawIndirectFirstInstance != features.drawIndirectFirstInstance)
	    {
	    	HG_CORE_ERROR("drawIndirectFirstInstance feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.depthClamp == VK_TRUE && gpu->PhysicalDeviceFeatures.depthClamp != features.depthClamp)
	    {
	    	HG_CORE_ERROR("depthClamp feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.depthBiasClamp == VK_TRUE && gpu->PhysicalDeviceFeatures.depthBiasClamp != features.depthBiasClamp)
	    {
	    	HG_CORE_ERROR("depthBiasClamp feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.fillModeNonSolid == VK_TRUE && gpu->PhysicalDeviceFeatures.fillModeNonSolid != features.fillModeNonSolid)
	    {
	    	HG_CORE_ERROR("fillModeNonSolid feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.depthBounds == VK_TRUE && gpu->PhysicalDeviceFeatures.depthBounds != features.depthBounds)
	    {
	    	HG_CORE_ERROR("depthBounds feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.wideLines == VK_TRUE && gpu->PhysicalDeviceFeatures.wideLines != features.wideLines)
	    {
	    	HG_CORE_ERROR("wideLines feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.largePoints == VK_TRUE && gpu->PhysicalDeviceFeatures.largePoints != features.largePoints)
	    {
	    	HG_CORE_ERROR("largePoints feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.alphaToOne == VK_TRUE && gpu->PhysicalDeviceFeatures.alphaToOne != features.alphaToOne)
	    {
	    	HG_CORE_ERROR("alphaToOne feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.multiViewport == VK_TRUE && gpu->PhysicalDeviceFeatures.multiViewport != features.multiViewport)
	    {
	    	HG_CORE_ERROR("multiViewport feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.samplerAnisotropy == VK_TRUE && gpu->PhysicalDeviceFeatures.samplerAnisotropy != features.samplerAnisotropy)
	    {
	    	HG_CORE_ERROR("samplerAnisotropy feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.textureCompressionETC2 == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionETC2 != features.textureCompressionETC2)
	    {
	    	HG_CORE_ERROR("textureCompressionETC2 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.textureCompressionASTC_LDR == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionASTC_LDR != features.textureCompressionASTC_LDR)
	    {
	    	HG_CORE_ERROR("textureCompressionASTC_LDR feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.textureCompressionBC == VK_TRUE && gpu->PhysicalDeviceFeatures.textureCompressionBC != features.textureCompressionBC)
	    {
	    	HG_CORE_ERROR("textureCompressionBC feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.occlusionQueryPrecise == VK_TRUE && gpu->PhysicalDeviceFeatures.occlusionQueryPrecise != features.occlusionQueryPrecise)
	    {
	    	HG_CORE_ERROR("occlusionQueryPrecise feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.pipelineStatisticsQuery == VK_TRUE && gpu->PhysicalDeviceFeatures.pipelineStatisticsQuery != features.pipelineStatisticsQuery)
	    {
	    	HG_CORE_ERROR("pipelineStatisticsQuery feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.vertexPipelineStoresAndAtomics == VK_TRUE && gpu->PhysicalDeviceFeatures.vertexPipelineStoresAndAtomics != features.vertexPipelineStoresAndAtomics)
	    {
	    	HG_CORE_ERROR("vertexPipelineStoresAndAtomics feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.fragmentStoresAndAtomics == VK_TRUE && gpu->PhysicalDeviceFeatures.fragmentStoresAndAtomics != features.fragmentStoresAndAtomics)
	    {
	    	HG_CORE_ERROR("fragmentStoresAndAtomics feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderTessellationAndGeometryPointSize == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderTessellationAndGeometryPointSize != features.shaderTessellationAndGeometryPointSize)
	    {
	    	HG_CORE_ERROR("shaderTessellationAndGeometryPointSize feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderImageGatherExtended == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderImageGatherExtended != features.shaderImageGatherExtended)
	    {
	    	HG_CORE_ERROR("shaderImageGatherExtended feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageExtendedFormats == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageExtendedFormats != features.shaderStorageImageExtendedFormats)
	    {
	    	HG_CORE_ERROR("shaderStorageImageExtendedFormats feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageMultisample == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageMultisample != features.shaderStorageImageMultisample)
	    {
	    	HG_CORE_ERROR("shaderStorageImageMultisample feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageReadWithoutFormat == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageReadWithoutFormat != features.shaderStorageImageReadWithoutFormat)
	    {
	    	HG_CORE_ERROR("shaderStorageImageReadWithoutFormat feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageWriteWithoutFormat == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageWriteWithoutFormat != features.shaderStorageImageWriteWithoutFormat)
	    {
	    	HG_CORE_ERROR("shaderStorageImageWriteWithoutFormat feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderUniformBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing != features.shaderUniformBufferArrayDynamicIndexing)
	    {
	    	HG_CORE_ERROR("shaderUniformBufferArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderSampledImageArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing != features.shaderSampledImageArrayDynamicIndexing)
	    {
	    	HG_CORE_ERROR("shaderSampledImageArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageBufferArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing != features.shaderStorageBufferArrayDynamicIndexing)
	    {
	    	HG_CORE_ERROR("shaderStorageBufferArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderStorageImageArrayDynamicIndexing == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing != features.shaderStorageImageArrayDynamicIndexing)
	    {
	    	HG_CORE_ERROR("shaderStorageImageArrayDynamicIndexing feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderClipDistance == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderClipDistance != features.shaderClipDistance)
	    {
	    	HG_CORE_ERROR("shaderClipDistance feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderCullDistance == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderCullDistance != features.shaderCullDistance)
	    {
	    	HG_CORE_ERROR("shaderCullDistance feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderFloat64 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderFloat64 != features.shaderFloat64)
	    {
	    	HG_CORE_ERROR("shaderFloat64 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderInt64 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderInt64 != features.shaderInt64)
	    {
	    	HG_CORE_ERROR("shaderInt64 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderInt16 == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderInt16 != features.shaderInt16)
	    {
	    	HG_CORE_ERROR("shaderInt16 feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderResourceResidency == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderResourceResidency != features.shaderResourceResidency)
	    {
	    	HG_CORE_ERROR("shaderResourceResidency feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.shaderResourceMinLod == VK_TRUE && gpu->PhysicalDeviceFeatures.shaderResourceMinLod != features.shaderResourceMinLod)
	    {
	    	HG_CORE_ERROR("shaderResourceMinLod feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseBinding == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseBinding != features.sparseBinding)
	    {
	    	HG_CORE_ERROR("sparseBinding feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyBuffer == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyBuffer != features.sparseResidencyBuffer)
	    {
	    	HG_CORE_ERROR("sparseResidencyBuffer feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyImage2D == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyImage2D != features.sparseResidencyImage2D)
	    {
	    	HG_CORE_ERROR("sparseResidencyImage2D feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyImage3D == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyImage3D != features.sparseResidencyImage3D)
	    {
	    	HG_CORE_ERROR("sparseResidencyImage3D feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency2Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency2Samples != features.sparseResidency2Samples)
	    {
	    	HG_CORE_ERROR("sparseResidency2Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency4Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency4Samples != features.sparseResidency4Samples)
	    {
	    	HG_CORE_ERROR("sparseResidency4Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency8Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency8Samples != features.sparseResidency8Samples)
	    {
	    	HG_CORE_ERROR("sparseResidency8Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidency16Samples == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidency16Samples != features.sparseResidency16Samples)
	    {
	    	HG_CORE_ERROR("sparseResidency16Samples feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.sparseResidencyAliased == VK_TRUE && gpu->PhysicalDeviceFeatures.sparseResidencyAliased != features.sparseResidencyAliased)
	    {
	    	HG_CORE_ERROR("sparseResidencyAliased feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.variableMultisampleRate == VK_TRUE && gpu->PhysicalDeviceFeatures.variableMultisampleRate != features.variableMultisampleRate)
	    {
	    	HG_CORE_ERROR("variableMultisampleRate feature not supported by physical device.");
	    	supported = false;
	    }

	    if (features.inheritedQueries == VK_TRUE && gpu->PhysicalDeviceFeatures.inheritedQueries != features.inheritedQueries)
	    {
	    	HG_CORE_ERROR("inheritedQueries feature not supported by physical device.");
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
		CreateSemaphores();
		CreateCommandPools();
		CreateCommandBuffers();
		CreateSwapChain();
		CreateRenderTargets();
		CreateRenderPass();
		CreateFrameBuffers();

		m_Initialized = true;
	}

	void GraphicsContext::DeinitializeImpl()
	{
		for (size_t i = 0; i < CommandPools.size(); i++) {
			vkFreeCommandBuffers(Device, CommandPools[i], 1, &CommandBuffers[i]);
		}

		for (auto framebuffer : FrameBuffers) {
			vkDestroyFramebuffer(Device, framebuffer, nullptr);
		}

		vkDestroyRenderPass(Device, RenderPass, nullptr);

		for (auto& image : SwapchainImages)
		{
			image.reset();
		}

		DepthImage.reset();
		ColorImage.reset();

		vkDestroySwapchainKHR(Device, Swapchain, nullptr);

		for (auto fence : CommandBufferFences)
		{
			vkDestroyFence(Device, fence, nullptr);
		}
		vkDestroyFence(Device, UploadFence, nullptr);

		for (size_t i = 0; i < CommandPools.size(); i++) {
			vkDestroyCommandPool(Device, CommandPools[i], nullptr);
		}
		vkDestroyCommandPool(Device, UploadCommandPool, nullptr);

		for (int i = 0; i < FrameCount; ++i)
		{
			vkDestroySemaphore(Device, AcquireSemaphores[i], nullptr);
			vkDestroySemaphore(Device, RenderCompleteSemaphores[i], nullptr);
		}

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
		CreateRenderTargets();
		CreateFrameBuffers();
		CreateCommandBuffers();
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

	void GraphicsContext::CreateSemaphores()
	{
		HG_PROFILE_FUNCTION();
		AcquireSemaphores.resize(FrameCount);
		RenderCompleteSemaphores.resize(FrameCount);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (int i = 0; i < FrameCount; ++i) {
			CheckVkResult(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &(AcquireSemaphores[i])));
			CheckVkResult(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &(RenderCompleteSemaphores[i])));
		}
	}

	void GraphicsContext::CreateCommandPools()
	{
		HG_PROFILE_FUNCTION();

		CommandPools.resize(FrameCount);
		for (int i = 0; i < FrameCount; ++i)
		{
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

			CheckVkResult(vkCreateCommandPool(Device, &commandPoolCreateInfo, nullptr, &CommandPools[i]));
		}

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

		CommandBuffers.resize(FrameCount);
		CommandBufferFences.resize(FrameCount);
		for (int i = 0; i < FrameCount; ++i)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

			// Don't worry about this
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			// The command pool we created above
			commandBufferAllocateInfo.commandPool = CommandPools[i];

			// We'll have two command buffers.  One will be in flight
			// while the other is being built.
			commandBufferAllocateInfo.commandBufferCount = 1;


			// You can allocate multiple command buffers at once.
			CheckVkResult(vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, &CommandBuffers[i]));

			// We create fences that we can use to wait for a 
			// given command buffer to be done on the GPU.
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			CheckVkResult(vkCreateFence(Device, &fenceCreateInfo, nullptr, &CommandBufferFences[i]));
		}

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
		HG_ASSERT(numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count.")

		// Second call uses numImages
		CheckVkResult(vkGetSwapchainImagesKHR(Device, Swapchain, &numImages, swapchainImages.data()));
		HG_ASSERT(numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count.");

		SwapchainImages.resize(FrameCount);

		// New concept - Image Views
		// Much like the logical device is an interface to the physical device,
		// image views are interfaces to actual images.  Think of it as this.
		// The image exists outside of you.  But the view is your personal view 
		// ( how you perceive ) the image.
		for (int i = 0; i < FrameCount; ++i) {
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

	void GraphicsContext::CreateRenderTargets()
	{
		HG_PROFILE_FUNCTION();

		VkFormat internalFormat;
		// Select Depth Format, prefer as high a precision as we can get.
		{
			VkFormat formats[] = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT
			};

			// Make sure to check it supports optimal tiling and is a depth/stencil format.
			internalFormat = ChooseSupportedFormat(
				formats, 2,
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
			internalFormat = VK_FORMAT_D32_SFLOAT;
		}

		// idTech4.5 does have an independent idea of a depth attachment
		// So now that the context contains the selected format we can simply
		// create the internal one.

		DepthImage = Image::Create(ImageType::Depth, GPU->SurfaceCapabilities.currentExtent.width, GPU->SurfaceCapabilities.currentExtent.height, 1, internalFormat, MSAASamples);
		if (CVar_MSAA.Get())
		{
			ColorImage = Image::Create(ImageType::SampledColorAttachment,
				GPU->SurfaceCapabilities.currentExtent.width,
				GPU->SurfaceCapabilities.currentExtent.height,
				1,
				SwapchainFormat,
				MSAASamples);
		}
	}

	void GraphicsContext::CreateRenderPass()
	{
		HG_PROFILE_FUNCTION();
		std::vector<VkAttachmentDescription> attachments;

		// VkNeo uses a single renderpass, so I just create it on startup.
		// Attachments act as slots in which to insert images.

		// For the color attachment, we'll simply be using the swapchain images.
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = SwapchainFormat;
		// Sample count goes from 1 - 64
		colorAttachment.samples = MSAASamples;
		// I don't care what you do with the image memory when you load it for use.
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// Just store the image when you go to store it.
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// I don't care what the initial layout of the image is.
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// It better be ready to present to the user when we're done with the renderpass.
		if (CVar_MSAA.Get()) 
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		else 
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments.push_back(colorAttachment);

		

		// For the depth attachment, we'll be using the _viewDepth we just created.
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = DepthImage->GetInternalFormat();
		depthAttachment.flags = 0;
		depthAttachment.samples = MSAASamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments.push_back(depthAttachment);

		if (CVar_MSAA.Get())
		{
			VkAttachmentDescription colorAttachmentResolve = {};
			colorAttachmentResolve.format = SwapchainFormat;
			colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			attachments.push_back(colorAttachmentResolve);
		}

		// Now we enumerate the attachments for a subpass.  We have to have at least one subpass.
		VkAttachmentReference colorRef = {};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef = {};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Basically is this graphics or compute
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;
		subpass.pDepthStencilAttachment = &depthRef;

		if (MSAASamples != VK_SAMPLE_COUNT_1_BIT)
		{
			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 2;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			subpass.pResolveAttachments = &colorAttachmentRef;
		}

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 0;

		if (CVar_MSAA.Get())
		{
			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			renderPassCreateInfo.dependencyCount = 1;
			renderPassCreateInfo.pDependencies = &dependency;
		}

		CheckVkResult(vkCreateRenderPass(Device, &renderPassCreateInfo, nullptr, &RenderPass));
	}

	void GraphicsContext::CreateFrameBuffers()
	{
		HG_PROFILE_FUNCTION();
		FrameBuffers.resize(FrameCount);

		
		std::vector<VkImageView> attachments;

		// Depth attachment is the same
		// We never show the depth buffer, so we only ever need one.
		int swapChainImageIndex;
		if (CVar_MSAA.Get())
		{
			attachments.resize(3);
			attachments[0] = ColorImage->GetImageView();
			swapChainImageIndex = 2;
		}
		else
		{
			attachments.resize(2);
			swapChainImageIndex = 0;
		}
		
		attachments[1] = DepthImage->GetImageView();

		// VkFrameBuffer is what maps attachments to a renderpass.  That's really all it is.
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// The renderpass we just created.
		frameBufferCreateInfo.renderPass = RenderPass;
		// The color and depth attachments
		frameBufferCreateInfo.attachmentCount = (uint32_t)attachments.size();
		frameBufferCreateInfo.pAttachments = attachments.data();
		// Current render size
		frameBufferCreateInfo.width = SwapchainExtent.width;
		frameBufferCreateInfo.height = SwapchainExtent.height;
		frameBufferCreateInfo.layers = 1;

		// Because we're double buffering, we need to create the same number of framebuffers.
		// The main difference again is that both of them use the same depth image view.
		for (int i = 0; i < FrameCount; ++i) {
			attachments[swapChainImageIndex] = SwapchainImages[i]->GetImageView();
			CheckVkResult(vkCreateFramebuffer(Device, &frameBufferCreateInfo, NULL, &FrameBuffers[i]));
		}
	}

	void GraphicsContext::CleanupSwapChain()
	{
		HG_PROFILE_FUNCTION();
		for (size_t i = 0; i < FrameBuffers.size(); i++) {
			vkDestroyFramebuffer(Device, FrameBuffers[i], nullptr);
		}

		for (size_t i = 0; i < CommandPools.size(); i++) {
			vkFreeCommandBuffers(Device, CommandPools[i], 1, &CommandBuffers[i]);
		}
		
		for (auto fence : CommandBufferFences)
		{
			vkDestroyFence(Device, fence, nullptr);
		}
		vkDestroyFence(Device, UploadFence, nullptr);

		for (size_t i = 0; i < SwapchainImages.size(); i++) {
			SwapchainImages[i].reset();
		}

		DepthImage.reset();
		ColorImage.reset();
	}

	VkSampleCountFlagBits GraphicsContext::GetMaxMSAASampleCount()
	{
		VkSampleCountFlags counts = GPU->DeviceProperties.limits.framebufferColorSampleCounts & GPU->DeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}
}
