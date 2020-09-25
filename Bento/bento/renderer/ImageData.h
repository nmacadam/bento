#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace bento
{
	struct ImageData
	{
		vk::UniqueImage image;
		//vk::Image image;
		//vk::UniqueDeviceMemory memory;
		VmaAllocation allocation;
		/*VmaAllocator allocator;

		std::unique_ptr<VmaAllocation, void(*)(VmaAllocation*)> allocation;

		void setAllocation(VmaAllocation allocation)
		{
			
		}

		void freeAllocation(VmaAllocation* allocation)
		{
			vmaDestroyImage(allocator, static_cast<VkImage>(image.release()), *allocation);
		}*/
	};

}
