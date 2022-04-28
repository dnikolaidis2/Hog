#include "hgpch.h"

#include "Hog/Core/Application.h"
#include "Hog/Core/Log.h"
#include "Hog/Core/Input.h"
#include "Hog/Renderer/GraphicsContext.h"
#include "Hog/Renderer/Renderer.h"

#include <GLFW/glfw3.h>

namespace Hog {

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name, ApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		HG_PROFILE_FUNCTION();

		HG_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		m_Window = Window::Create(WindowProps(name));
		m_Window->SetEventCallback(HG_BIND_EVENT_FN(Application::OnEvent));
	}

	Application::~Application()
	{
		HG_PROFILE_FUNCTION();
	}

	void Application::PushLayer(Ref<Layer> layer)
	{
		HG_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Ref<Layer> layer)
	{
		HG_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::PopLayer(Ref<Layer> layer)
	{
		HG_PROFILE_FUNCTION();

		m_LayerStack.PopLayer(layer);
	}

	void Application::PopOverlay(Ref<Layer> layer)
	{
		HG_PROFILE_FUNCTION();

		m_LayerStack.PopOverlay(layer);
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		HG_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HG_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(HG_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled) 
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		HG_PROFILE_FUNCTION();

		while (m_Running)
		{
			HG_PROFILE_START_FRAME("MainThread");

			float time = (float)glfwGetTime();
			m_Timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				if (m_ImGuiLayer)
				{
					m_ImGuiLayer->Begin();
					{
						HG_PROFILE_SCOPE("LayerStack OnImGuiRender");

						for (Ref<Layer> layer : m_LayerStack)
							layer->OnImGuiRender();
					}
					m_ImGuiLayer->End();
				}

				{
					HG_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(m_Timestep);
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
		HG_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;

		return false;
	}

}
