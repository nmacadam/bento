#include "Engine.h"
#include <GLFW/glfw3.h>


Engine::Engine()
{
}


Engine::~Engine()
{
}

void Engine::initialize(const char* title, int screenWidth, int screenHeight)
{
	glfwInit();

	window.initialize(title, screenWidth, screenHeight);
	renderer.initialize(&window);

	// set window resize callback for renderer
	glfwSetWindowUserPointer(window.getHandle(), &renderer);
	glfwSetFramebufferSizeCallback(window.getHandle(), Renderer::framebufferResizeCallback);
}

void Engine::run()
{
	while (!glfwWindowShouldClose(window.getHandle())) {
		glfwPollEvents();
		update();
		render();
	}
}

void Engine::clean()
{
	renderer.clean();
	glfwTerminate();
}

void Engine::update()
{ }

void Engine::render()
{
	renderer.drawFrame();
}
