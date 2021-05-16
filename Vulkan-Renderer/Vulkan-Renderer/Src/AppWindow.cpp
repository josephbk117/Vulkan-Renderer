#include "AppWindow.h"
#include <stdexcept>
#include "ConstantsAndDefines.h"
#include "Utils.h"

void ApplicationWindow::AppWindow::Initwindow(const WindowProperties& _windowProps)
{
	using namespace Utilities;
	PROFILE_FUNCTION();

	this->windowProps = _windowProps;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	windowPtr = glfwCreateWindow(_windowProps.width, _windowProps.height, _windowProps.windowName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(windowPtr, this);
	glfwSetFramebufferSizeCallback(windowPtr, FrameBufferResizeCallback);
}

ApplicationWindow::AppWindow::WindowProperties ApplicationWindow::AppWindow::GetWindowProperties() const
{
	return windowProps;
}

bool ApplicationWindow::AppWindow::ShouldClose() const
{
	using namespace Utilities;
	PROFILE_FUNCTION();

	return glfwWindowShouldClose(windowPtr);
}

void ApplicationWindow::AppWindow::PollInputs() const
{
	using namespace Utilities;
	PROFILE_FUNCTION();

	glfwPollEvents();
}

void ApplicationWindow::AppWindow::CreateVulkanWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	using namespace Utilities;
	PROFILE_FUNCTION();

	if (glfwCreateWindowSurface(instance, windowPtr, nullptr, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vulkan window surface");
	}
}

GLFWwindow* ApplicationWindow::AppWindow::GetWindow() const
{
	return windowPtr;
}

bool ApplicationWindow::AppWindow::HasWindowBeenResized() const
{
	return frameBufferResized;
}

void ApplicationWindow::AppWindow::ResetWindowResizedState()
{
	frameBufferResized = false;
}

void ApplicationWindow::AppWindow::FrameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<ApplicationWindow::AppWindow*>(glfwGetWindowUserPointer(window));
	app->frameBufferResized = true;
}

ApplicationWindow::AppWindow::~AppWindow()
{
	using namespace Utilities;
	PROFILE_FUNCTION();

	glfwDestroyWindow(windowPtr);
	glfwTerminate();
}