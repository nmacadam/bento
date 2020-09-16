#pragma once
#include <vulkan/vulkan.hpp>
//#include <vk_mem_alloc.h>

struct BufferData
{
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory memory;
	//VkBuffer buffer;
	//VmaAllocation allocation;
};
