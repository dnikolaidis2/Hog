#include "vkcpch.h"

#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Input.h"
#include "VulkanCore/Renderer/GraphicsContext.h"
#include "VulkanCore/Renderer/Renderer.h"

#include <GLFW/glfw3.h>

namespace VulkanCore {

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name, ApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		VKC_PROFILE_FUNCTION();

		VKC_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		m_Window = Window::Create(WindowProps(name));
		m_Window->SetEventCallback(VKC_BIND_EVENT_FN(Application::OnEvent));
	}

	Application::~Application()
	{
		VKC_PROFILE_FUNCTION();
	}

	void Application::PushLayer(Ref<Layer> layer)
	{
		VKC_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Ref<Layer> layer)
	{
		VKC_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::PopLayer(Ref<Layer> layer)
	{
		VKC_PROFILE_FUNCTION();

		m_LayerStack.PopLayer(layer);
	}

	void Application::PopOverlay(Ref<Layer> layer)
	{
		VKC_PROFILE_FUNCTION();

		m_LayerStack.PopOverlay(layer);
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		VKC_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(VKC_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(VKC_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled) 
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		VKC_PROFILE_FUNCTION();

		while (m_Running)
		{
			VKC_PROFILE_START_FRAME("MainThread");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				if (m_ImGuiLayer)
				{
					m_ImGuiLayer->Begin();
					{
						VKC_PROFILE_SCOPE("LayerStack OnImGuiRender");

						for (Ref<Layer> layer : m_LayerStack)
							layer->OnImGuiRender();
					}
					m_ImGuiLayer->End();
				}

				{
					VKC_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}
			}

			m_Window->OnUpdate();
		}

		m_ImGuiLayer.reset();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		VKC_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;

		return false;
	}

}
