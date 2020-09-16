#include <vulkan/vulkan.hpp>

//#define GLFW_INCLUDE_VULKAN
//#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include "Engine.h"

int main() {
	Engine engine;

	engine.initialize("bento", 800, 600);

	try 
	{
		engine.run();
	}
	catch (vk::SystemError & err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		exit(-1);
	}
	catch (std::exception & err)
	{
		std::cerr << err.what() << std::endl;
		exit(-1);
	}

	engine.clean();

	return 0;
}
