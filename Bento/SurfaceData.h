#pragma once
#include <vulkan/vulkan.hpp>
#include "Window.h"

struct SurfaceData
{
	SurfaceData(vk::UniqueInstance & instance, std::string const & windowName, vk::Extent2D const & extent);

	vk::Extent2D         extent;
	Window				 window;
	vk::UniqueSurfaceKHR surface;
};
