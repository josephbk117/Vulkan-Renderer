#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace ApplicationWindow
{
	class AppWindow
	{
	public:

		struct WindowProperties
		{
			std::string windowName = "Default name";
			int width = 800;
			int height = 640;

			WindowProperties() = default;
			~WindowProperties() = default;
			WindowProperties(int _width, int _height, const std::string _windowName) : width{ _width }, height(_height), windowName{ _windowName } {}
		};

		AppWindow() = default;
		AppWindow(const AppWindow&) = delete;
		AppWindow& operator=(const AppWindow&) = delete;
		~AppWindow();

		void Initwindow(const WindowProperties& _windowProps);
		WindowProperties GetWindowProperties() const;
		bool ShouldClose() const;
		void PollInputs() const;
		void CreateVulkanWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	protected:
		WindowProperties windowProps;

	private:
		GLFWwindow* windowPtr = nullptr;
	};
}
