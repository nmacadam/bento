//#pragma once
//#include <vk_mem_alloc.h>
//
//class UniqueAllocation 
//{
//public:
//
//	UniqueAllocation(VmaAllocator allocator, VmaAllocation allocation)
//	{
//		this->allocator = allocator;
//		this->allocation = allocation;
//	}
//
//	~UniqueAllocation()
//	{
//		vmaDestroyImage(allocator, static_cast<VkImage>(image.release()), *allocation);
//	}
//
//	VmaAllocation* get() { return &allocation; }
//
//private:
//	VmaAllocator allocator;
//	VmaAllocation allocation;
//};
