#pragma once
#include <vulkan/vulkan.hpp>

struct ImageData
{
	vk::UniqueImage image;
	vk::UniqueDeviceMemory memory;
};
