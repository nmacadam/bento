#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>

// Details basic surface capabilities, formats, and presentation modes
struct SwapChainSupportDetails 
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};
