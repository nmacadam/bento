#pragma once
#include <vulkan/vulkan.hpp>

extern bento::application* bento::createApplication();

int main(int argc, char** argv)
{
	bento::log::initialize();

	auto app = bento::createApplication();
	app->initialize("Vulkan window", 800, 600);
	try
	{
		app->run();
	}
	catch (vk::SystemError & err)
	{
		bento::log::error("vk::SystemError: {}", err.what());
		exit(-1);
	}
	catch (std::exception & err)
	{
		bento::log::error(err.what());
		exit(-1);
	}
	app->clean();
	delete app;

	return 0;
}