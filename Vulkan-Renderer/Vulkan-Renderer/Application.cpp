#include "Application.h"

Application::Application()
{
	appWindow.Initwindow({ 1200, 800, "Vulkan Test Window" });
}

void Application::Run()
{
	while (!appWindow.ShouldClose())
	{
		appWindow.PollInputs();
	}
}
