#pragma once

#include "Hog/Core/Base.h"

#include "Hog/Core/Window.h"
#include "Hog/Core/LayerStack.h"
#include "Hog/Events/Event.h"
#include "Hog/Events/ApplicationEvent.h"

#include "Hog/Core/Timestep.h"

#include "Hog/ImGui/ImGuiLayer.h"

int main(int argc, char** argv);

namespace Hog {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			HG_CORE_ASSERT(index < Count);
			return Args[index];
		}
	};

	class Application
	{
	public:
		Application(const std::string& name = "Hog App", ApplicationCommandLineArgs args = ApplicationCommandLineArgs());
		virtual ~Application();

		void OnEvent(Event& e);

		void PushLayer(Ref<Layer> layer);
		void PushOverlay(Ref<Layer> layer);
		void PopLayer(Ref<Layer> layer);
		void PopOverlay(Ref<Layer> layer);

		Window& GetWindow() { return *m_Window; }

		void Close();

		void SetImGuiLayer(Ref<ImGuiLayer> layer) { m_ImGuiLayer = layer; PushOverlay(layer); }
		Ref<ImGuiLayer> GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }

		ApplicationCommandLineArgs GetCommandLineArgs() const { return m_CommandLineArgs; }
		Timestep GetTimestep() const { return m_Timestep; }
	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		ApplicationCommandLineArgs m_CommandLineArgs;
		Scope<Window> m_Window;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		Ref<ImGuiLayer> m_ImGuiLayer;
		float m_LastFrameTime = 0.0f;
		Timestep m_Timestep;
	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in CLIENT
	Application* CreateApplication(ApplicationCommandLineArgs args);

}
