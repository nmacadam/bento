#pragma once
#include <GLFW/glfw3.h>

class Window
{
public:
	Window();
	~Window();

	void initialize(const char* title, int screenWidth, int screenHeight);

	int getWidth() const
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		return width;
	}
	int getHeight() const
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		return height;
	}

	GLFWwindow* getHandle() const { return window; };

private:
	GLFWwindow* window;
};

