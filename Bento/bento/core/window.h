#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace bento
{
	class window
	{
	public:
		window();
		~window();

		void initialize(const char* title, int screenWidth, int screenHeight);

		int getWidth() const
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(glfwWindow, &width, &height);
			return width;
		}
		int getHeight() const
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(glfwWindow, &width, &height);
			return height;
		}

		GLFWwindow* getHandle() const { return glfwWindow; };

	private:
		GLFWwindow* glfwWindow;
	};
}

