#pragma once
#include "AppWindow.h"
#include "RenderPipeline.h"
#include "VulkanRenderer.h"

using namespace ApplicationWindow;
using namespace Renderer;

class Application
{
public:
	Application();
	~Application() = default;
	void Run();
private:
	AppWindow appWindow;
	VulkanRenderer renderer;
};

