#pragma once
#include <vulkan/vulkan.hpp>
//#include <vk_mem_alloc.h>

namespace bento
{
	struct BufferData
	{
		vk::UniqueBuffer buffer;
		vk::UniqueDeviceMemory memory;
		//VmaAllocation allocation;
		vk::Device device;

		void* mapped = nullptr;
		vk::Result map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
		{
			return device.mapMemory(memory.get(), offset, size, {}, &mapped);
		}

		void unmap()
		{
			if (mapped)
			{
				device.unmapMemory(memory.get());
				mapped = nullptr;
			}
		}

		vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
		{
			vk::MappedMemoryRange mappedRange(memory.get(), offset, size);
			return device.flushMappedMemoryRanges(1, &mappedRange);
		}

		void destroy()
		{
			if (buffer.get())
			{
				device.destroyBuffer(buffer.release());
			}
			if (memory.get())
			{
				device.freeMemory(memory.release());
			}
		}
	};
}
