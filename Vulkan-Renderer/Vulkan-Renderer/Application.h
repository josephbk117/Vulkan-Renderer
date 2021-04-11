#pragma once
#include "AppWindow.h"

using namespace ApplicationWindow;

class Application
{
public: 
	Application();
	~Application() = default;
	void Run();
private: 
	AppWindow appWindow;
};

