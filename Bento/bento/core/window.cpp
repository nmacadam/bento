#include "window.h"
#include <stdexcept>


namespace bento
{
	window::window()
	{
	}


	window::~window()
	{
	}

	void window::initialize(const char* title, int screenWidth, int screenHeight)
	{
		// initialize glfw without opengl
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// create window
		glfwWindow = glfwCreateWindow(screenWidth, screenHeight, title, nullptr, nullptr);
		if (glfwWindow == nullptr)
		{
			throw std::runtime_error("failed to create GLFW window!");
		}
	}
}
