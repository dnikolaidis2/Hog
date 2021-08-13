#pragma once

#include <vulkan/vulkan.h>

static void CheckResult(VkResult result)
{
	if (result != 0)
		VKC_CORE_ERROR("Vulkan check failed with code: {0}", result);
		VKC_CORE_ASSERT(false)
}

namespace GraphicsContext {

	struct ContextData
	{
		VkApplicationInfo ApplicationInfo =	{	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
												.pApplicationName = "Test app",
												.applicationVersion = 1,
												.pEngineName = "Hazel",
												.engineVersion = 1,
												.apiVersion = VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION) };

		std::vector<const char*> InstanceExtensions = {};
		std::vector<const char*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		bool EnableValidationLayers = true;
		bool DebugInstanceExtension = false; // Enables VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

		VkInstance Instance;
	};

	static ContextData s_Context;

	static void CreateInstance()
	{
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &(s_Context.ApplicationInfo);

		bool appendDefaultDeviceExtension = true;
		for (auto deviceExtension : s_Context.DeviceExtensions)
			if (std::strcmp(deviceExtension, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
				appendDefaultDeviceExtension = false;

		if (appendDefaultDeviceExtension)
			s_Context.DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (s_Context.EnableValidationLayers)
		{
			if (s_Context.DebugInstanceExtension)
				s_Context.ValidationLayers.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		else
		{
			s_Context.ValidationLayers.clear();
		}

		// Give all the extensions/layers to the create info.
		createInfo.enabledExtensionCount = s_Context.InstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = s_Context.InstanceExtensions.data();
		createInfo.enabledLayerCount = s_Context.ValidationLayers.size();
		createInfo.ppEnabledLayerNames = s_Context.ValidationLayers.data();
		CheckResult(vkCreateInstance(&createInfo, NULL, &s_Context.Instance));
	}

	static void DestroyInstance()
	{
		vkDestroyInstance(s_Context.Instance, nullptr);
	}

	static void Init()
	{
		CreateInstance();
	}

	/*
	class GraphicsContext
	{
	public:
		static GraphicsContext& Get()
		{
			static GraphicsContext instance;

			return instance;
		}

		static void Init() { Get().InitImpl(); }
		static ContextData& GetContextData() { Get().m_Data; }
	public:
		GraphicsContext(GraphicsContext const&) = delete;
		void operator=(GraphicsContext const&) = delete;
	private:
		GraphicsContext() = default;

		void InitImpl();
		void CreateInstance();
	private:
		static bool s_Initialized = false;
	private:
		ContextData m_Data;

	};
	*/
}