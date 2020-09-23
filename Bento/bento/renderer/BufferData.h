#pragma once
#include <vulkan/vulkan.hpp>

namespace bento
{
	struct BufferData
	{
		vk::UniqueBuffer buffer;
		vk::UniqueDeviceMemory memory;
	};
}