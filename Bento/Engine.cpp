#include "Engine.h"
#include <GLFW/glfw3.h>
#include "Mesh.h"
#include <ctime>


Engine::Engine()
{
}


Engine::~Engine()
{
}

void Engine::initialize(const char* title, int screenWidth, int screenHeight)
{
	srand(static_cast <unsigned> (time(0)));
	glfwInit();

	window.initialize(title, screenWidth, screenHeight);
	renderer.initialize(&window);

	// set window resize callback for renderer
	glfwSetWindowUserPointer(window.getHandle(), &renderer);
	glfwSetFramebufferSizeCallback(window.getHandle(), Renderer::framebufferResizeCallback);

	//MeshFactory meshFactory(&renderer.context);
	//auto& cube = renderer.meshFactory.create(Cube::vertices, Cube::indices);
	auto& plane = renderer.meshFactory.create(Plane::vertices, Plane::indices, glm::vec3(1.0f, 0.0f, 0.0f));
	auto& quad = renderer.meshFactory.create(Quad::vertices, Quad::indices, glm::vec3(0.0f, 0.0f, 0.0f));

	renderer.rebuildCommandBuffers();
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
