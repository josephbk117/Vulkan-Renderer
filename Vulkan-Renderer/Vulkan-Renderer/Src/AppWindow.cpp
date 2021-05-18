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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	windowPtr = glfwCreateWindow(_windowProps.width, _windowProps.height, _windowProps.windowName.c_str(), nullptr, nullptr);
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

GLFWwindow* ApplicationWindow::AppWindow::GetWindow() const
{
	return windowPtr;
}

ApplicationWindow::AppWindow::~AppWindow()
{
	using namespace Utilities;
	PROFILE_FUNCTION();

	glfwDestroyWindow(windowPtr);
	glfwTerminate();
}