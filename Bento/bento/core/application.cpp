#include "application.h"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "log.h"

namespace bento
{
	application::application()
	{
	}


	application::~application()
	{

	}

	void application::initialize(const char* title, int screenWidth, int screenHeight)
	{
		glfwInit();
		window.initialize(title, screenWidth, screenHeight);

		log::info("initialized");
	}

	void application::run()
	{

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		log::info("extensions supported: {0}", extensionCount);

		glm::mat4 matrix;
		glm::vec4 vec;
		auto test = matrix * vec;

		start();
		while (!glfwWindowShouldClose(window.getHandle()))
		{
			glfwPollEvents();
			update();
			render();
		}
	}

	void application::clean()
	{
		glfwTerminate();
		log::info("cleaned");
		system("pause\n");
	}

	void application::pushState(state* state)
	{
		stack.push(state);
	}

	void application::popState()
	{
		stack.pop();
	}

	void application::start()
	{
		stack.top()->start();
	}

	void application::update()
	{
		stack.top()->update();
	}

	void application::render()
	{
		stack.top()->render();
	}
}
