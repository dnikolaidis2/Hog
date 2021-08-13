/*
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "VulkanCore/Core/Log.h"

static void GLFWErrorCallback(int error, const char* description)
{
    std::cout << "GLFW Error (" << error << "): " << description << std::endl;
}

int main() {
    glfwInit();

	glfwSetErrorCallback(GLFWErrorCallback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
*/


#include <VulkanCore.h>
#include <VulkanCore/Core/EntryPoint.h>

#include "SandboxLayer.h"

class Sandbox : public VulkanCore::Application
{
public:
	Sandbox(VulkanCore::ApplicationCommandLineArgs args)
	{
		PushLayer(new SandboxLayer());
	}

	~Sandbox()
	{
	}
};

VulkanCore::Application* VulkanCore::CreateApplication(VulkanCore::ApplicationCommandLineArgs args)
{
	return new Sandbox(args);
}
