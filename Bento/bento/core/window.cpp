#include "bpch.h"
#include "Window.h"
#include <stdexcept>


namespace bento
{
	Window::Window()
	{
	}


	Window::~Window()
	{
	}

	void Window::initialize(const char* title, int screenWidth, int screenHeight)
	{
		// initialize glfw without opengl
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// create Window
		glfwWindow = glfwCreateWindow(screenWidth, screenHeight, title, nullptr, nullptr);
		if (glfwWindow == nullptr)
		{
			throw std::runtime_error("failed to create GLFW Window!");
		}
	}
}
