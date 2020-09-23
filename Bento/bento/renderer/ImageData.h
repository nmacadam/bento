#pragma once
#include <vulkan/vulkan.hpp>

namespace bento
{
	struct ImageData
	{
		vk::UniqueImage image;
		vk::UniqueDeviceMemory memory;
	};

}