#include "Window.h"
#include <stdexcept>


Window::Window()
{ }


Window::~Window()
{
	glfwDestroyWindow(window);
}

void Window::initialize(const char* title, int screenWidth, int screenHeight)
{
	// initialize glfw without opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// create window
	window = glfwCreateWindow(screenWidth, screenHeight, title, nullptr, nullptr);
	if (window == nullptr)
	{
		throw std::runtime_error("failed to create GLFW window!");
	}
}
