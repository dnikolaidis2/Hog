#pragma once

#include "VulkanCore/Core/Window.h"

#include <GLFW/glfw3.h>

namespace VulkanCore {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		uint32_t GetWidth() const override { return m_Data.Width; }
		uint32_t GetHeight() const override { return m_Data.Height; }
		uint32_t GetFrameBufferWidth() const override { return m_Data.FrameBufferWidth; }
		uint32_t GetFrameBufferHeight() const override { return m_Data.FrameBufferHeight; }

		// Window attributes
		void CreateSurface(VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) override { VKC_PROFILE_FUNCTION(); glfwCreateWindowSurface(instance, m_Window, allocator, surface); };
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		virtual void* GetNativeWindow() const { return m_Window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			uint32_t FrameBufferWidth;
			uint32_t FrameBufferHeight;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}