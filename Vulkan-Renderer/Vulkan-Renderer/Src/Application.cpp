#include "Application.h"
#include "VulkanRenderer.h"
#include "ConstantsAndDefines.h"

Application::Application()
{
	Benchmark::Get().BeginSession("Profile");
	PROFILE_FUNCTION();
	appWindow.Initwindow({ 1200, 800, "Vulkan Test Window" });
}

void Application::Run()
{
	PROFILE_FUNCTION();

	renderer.Init(appWindow.GetWindow());

	{
		PROFILE_SCOPE("RenderLoop");
		while (!appWindow.ShouldClose())
		{
			appWindow.PollInputs();
			renderer.Draw();
		}
	}

	renderer.CleanUp();
	Benchmark::Get().EndSession();
}
