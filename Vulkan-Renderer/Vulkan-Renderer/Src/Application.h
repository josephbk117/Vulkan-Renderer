#pragma once
#include "AppWindow.h"
#include "RenderPipeline.h"

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
	RenderPipeline renderPipeline{ "simple_shader.vert", "simple_shader.frag" };
};

