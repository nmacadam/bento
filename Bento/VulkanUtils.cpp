#include "VulkanUtils.h"
#include <iostream>
#include <vulkan/vulkan.h>

VkDebugUtilsMessengerEXT VulkanUtils::createDebugUtilsMessenger(vk::UniqueInstance& instance)
{
	VkDebugUtilsMessengerCreateInfoEXT vkCreateInfo;
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	vkCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	vkCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	vkCreateInfo.pfnUserCallback = debugCallback;
	vkCreateInfo.flags = VkDebugUtilsMessengerCreateFlagsEXT();

	//vk::DebugUtilsMessengerCreateInfoEXT createInfo(vkCreateInfo);
	VkDebugUtilsMessengerEXT debugMessenger;

	if (CreateDebugUtilsMessengerEXT(instance->operator VkInstance_T*(), &vkCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}

	return debugMessenger;
}

BufferData VulkanUtils::createBuffer(vk::Device device, vk::DeviceSize size, vk::PhysicalDevice physicalDevice, vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags memoryProperties)
{
	BufferData data;

	// create the buffer
	vk::BufferCreateInfo bufferCreateInfo(
		{},
		size,
		usage,
		vk::SharingMode::eExclusive
	);
	data.buffer = device.createBufferUnique(bufferCreateInfo);

	// allocate memory for the buffer
	vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(data.buffer.get());
	vk::MemoryAllocateInfo memoryAllocateInfo(
		memoryRequirements.size,
		VulkanUtils::findMemoryType(
			physicalDevice.getMemoryProperties(),
			memoryRequirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		)
	);

	// this bad! use VMA!
	data.memory = device.allocateMemoryUnique(memoryAllocateInfo);

	// bind memory to the buffer
	// - offset: if nonzero, must be divisible by memRequirements.alignment
	device.bindBufferMemory(data.buffer.get(), data.memory.get(), 0);

	return data;
}

ImageData VulkanUtils::createImage(vk::Device device, vk::PhysicalDevice physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	ImageData data;

	// specify image data to create a image from
	vk::ImageCreateInfo imageInfo(
		{},
		vk::ImageType::e2D,
		format,
		vk::Extent3D(width, height, 1),
		1,
		1,
		vk::SampleCountFlagBits::e1,
		tiling,
		usage,
		vk::SharingMode::eExclusive
	);

	// create image
	data.image = device.createImageUnique(imageInfo);

	// collect memory requirements and allocation info for image memory
	vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(data.image.get());
	vk::MemoryAllocateInfo allocateInfo(
		memoryRequirements.size,
		VulkanUtils::findMemoryType(physicalDevice.getMemoryProperties(), memoryRequirements.memoryTypeBits, properties)
	);

	// allocate memory
	data.memory = device.allocateMemoryUnique(allocateInfo);

	// bind image to memory
	device.bindImageMemory(data.image.get(), data.memory.get(), 0);

	return data;
}

vk::UniqueImageView VulkanUtils::createImageView(vk::Device device, vk::Image image, vk::Format format)
{
	// apply a standard component mapping (for swizzling)
	const vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
	// subresource range describes the image's purpose and which parts to access
	// our image is a color target without mipmapping or multiple layers (for now)
	const vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

	// create a new image view
	const vk::ImageViewCreateInfo imageViewCreateInfo(
		vk::ImageViewCreateFlags(),
		image,
		vk::ImageViewType::e2D,
		format,
		componentMapping,
		subResourceRange
	);

	return device.createImageViewUnique(imageViewCreateInfo);
}

VkResult VulkanUtils::CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanUtils::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

VkBool32 __stdcall VulkanUtils::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

SwapChainSupportDetails VulkanUtils::querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
	SwapChainSupportDetails details;

	// query surface capabilities, formats, and present modes
	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

uint32_t VulkanUtils::findMemoryType(vk::PhysicalDeviceMemoryProperties const& memoryProperties, uint32_t typeFilter,
	vk::MemoryPropertyFlags requirementsMask)
{
	uint32_t typeIndex = uint32_t(~0);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((typeFilter & 1) &&
			((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask))
		{
			typeIndex = i;
			return typeIndex;
		}
		typeFilter >>= 1;
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanUtils::copyBuffer(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
	// could be optimized for multiple transfers at once
	// memory transfer operations are executed using command buffers

	// begin recording commands
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, pool);

	// copy the source buffer to the destination buffer
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));

	// stop recording commands
	endSingleTimeCommands(commandBuffer, device, pool, queue);
}

void VulkanUtils::copyBufferToImage(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::Buffer srcBuffer,
	vk::Image dstImage, uint32_t width, uint32_t height)
{
	// begin recording commands
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, pool);

	vk::BufferImageCopy region(
		0,
		0,
		0, 
		vk::ImageSubresourceLayers(
			vk::ImageAspectFlagBits::eColor,
			0,
			0,
			1
		), 
		vk::Offset3D(0, 0, 0),
		vk::Extent3D(width, height, 1)
	);
	commandBuffer.copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &region);

	// stop recording commands
	endSingleTimeCommands(commandBuffer, device, pool, queue);
}

//vk::UniqueDebugUtilsMessengerEXT VulkanUtils::createDebugUtilsMessenger(vk::UniqueInstance& instance)
//{
//	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
//		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
//	vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
//		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
//		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
//	return instance->createDebugUtilsMessengerEXTUnique(vk::DebugUtilsMessengerCreateInfoEXT(
//		{}, severityFlags, messageTypeFlags, &debugUtilsMessengerCallback));
//}
//
//VKAPI_ATTR VkBool32 VKAPI_CALL __stdcall VulkanUtils::debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//	VkDebugUtilsMessageTypeFlagsEXT messageTypes, VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*)
//{
//	//std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
//
//	return false;
//}


bool VulkanUtils::checkLayers(std::vector<const char*> const& layers,
	std::vector<vk::LayerProperties> const& properties)
{
	// return true if all layers are listed in the properties
	return std::all_of(layers.begin(), layers.end(), [&properties](char const * name) {
		return std::find_if(properties.begin(), properties.end(), [&name](vk::LayerProperties const & property) {
			return strcmp(property.layerName, name) == 0;
		}) != properties.end();
	});
}

// requires a subsequent call to endSingleTimeCommands!
vk::CommandBuffer VulkanUtils::beginSingleTimeCommands(vk::Device device, vk::CommandPool pool)
{
	// we need to allocate a temporary command buffer
	vk::CommandBufferAllocateInfo allocateInfo(
		pool,
		vk::CommandBufferLevel::ePrimary,
		1
	);

	vk::CommandBuffer commandBuffer;
	device.allocateCommandBuffers(&allocateInfo, &commandBuffer);

	// begin recording command buffer
	// we're only going to use the buffer once and wait with returning from the function or until the copy has finished
	// we'll inform the driver about our intent with OneTimeSubmit
	commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	return commandBuffer;
}

void VulkanUtils::endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Device device, vk::CommandPool pool, vk::Queue queue)
{
	// end command recording
	commandBuffer.end();

	// submit the commands for execution and wait
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	queue.submit(1, &submitInfo, nullptr);
	queue.waitIdle();

	// free the temporary command buffer
	device.freeCommandBuffers(pool, 1, &commandBuffer);
}

void VulkanUtils::transitionImageLayout(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
	vk::ImageLayout newLayout)
{
	// begin recording commands
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, pool);

	// we'll use an image barrier to perform the layout transition
	// used to synchronize access to resources, (e.g. ensuring a write completes before reading),
	// but it can also be used to transition image layouts and transfer queue family ownership
	// - accessMasks ...
	// - oldLayout and newLayout should match our requested transition
	// - queue family indices would be specified here if we wanted to transfer queue family ownership
	// - the subresource range specifies which subresources are affected, but since our image is not an array and does not have mipmapping,
	//		only one level and layer are specified
	vk::ImageMemoryBarrier barrier(
		{},
		{},
		oldLayout,
		newLayout, 
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		image, 
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	);

	// set access mask and pipeline stages based on the layout in the transition
	// - undefined->transfer destination, don't need to wait
	// - transfer destination->shader, shader reads should wait on transfer writes
	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	// note: if the transition is not defined here it will result in an error
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	// run the barrier
	commandBuffer.pipelineBarrier(
		sourceStage,
		destinationStage, 
		vk::DependencyFlags(),
		0,
		nullptr, 
		0,
		nullptr,
		1,
		&barrier
	);

	// end command recording
	endSingleTimeCommands(commandBuffer, device, pool, queue);
}
