
#include "Application.h"
#include <iostream>
#include <stdexcept>
int main()
{
	Application app;

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}