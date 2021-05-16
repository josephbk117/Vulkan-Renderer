#include "Application.h"
#include "VulkanRenderer.h"
#include "ConstantsAndDefines.h"

Application::Application()
{
	Benchmark::Get().BeginSession("Profile");
	PROFILE_FUNCTION();
	appWindow.Initwindow({ 1920, 1080, "Vulkan Test Window" });
}

void Application::Run()
{
	PROFILE_FUNCTION();

	renderer.Init(&appWindow);
	int32_t planeModelId = renderer.CreateModel("11805_airplane_v2_L2.obj", 0.1f);
	{
		PROFILE_SCOPE("RenderLoop");
		while (!appWindow.ShouldClose())
		{
			static float angle = 0.0f;
			static float lastTime = 0.0f;

			float deltaTime = 0.0f;

			float now = static_cast<float>(glfwGetTime());
			deltaTime = now - lastTime;

			angle += deltaTime;

			glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, glm::sin(angle) * 50.0f));
			glm::mat4 rot = glm::rotate(translation, glm::radians(angle * 10.0f), GLOBAL_FORWARD);
			rot = glm::rotate(rot, glm::radians(angle * 20.0f), GLOBAL_UP);

			appWindow.PollInputs();
			renderer.Update(planeModelId, rot);
			renderer.Draw();

			lastTime = now;

		}
	}

	renderer.CleanUp();
	Benchmark::Get().EndSession();
}
