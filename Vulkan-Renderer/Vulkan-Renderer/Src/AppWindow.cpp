#include "AppWindow.h"
#include <stdexcept>

void ApplicationWindow::AppWindow::Initwindow(const WindowProperties& _windowProps)
{
	this->windowProps = _windowProps;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	windowPtr = glfwCreateWindow(_windowProps.width, _windowProps.height, _windowProps.windowName.c_str(), nullptr, nullptr);
}

ApplicationWindow::AppWindow::WindowProperties ApplicationWindow::AppWindow::GetWindowProperties() const
{
	return windowProps;
}

bool ApplicationWindow::AppWindow::ShouldClose() const
{
	return glfwWindowShouldClose(windowPtr);
}

void ApplicationWindow::AppWindow::PollInputs() const
{
	glfwPollEvents();
}

void ApplicationWindow::AppWindow::CreateVulkanWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, windowPtr, nullptr, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan window surface");
	}
}

ApplicationWindow::AppWindow::~AppWindow()
{
	glfwDestroyWindow(windowPtr);
	glfwTerminate();
}