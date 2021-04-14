#include "Application.h"
#include "VulkanRenderer.h"

Application::Application()
{
	appWindow.Initwindow({ 1200, 800, "Vulkan Test Window" });
}

void Application::Run()
{
	renderer.Init(appWindow.GetWindow());

	while (!appWindow.ShouldClose())
	{
		appWindow.PollInputs();
	}

	renderer.CleanUp();
}
