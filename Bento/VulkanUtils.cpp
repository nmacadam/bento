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

	// we need to allocate a temporary command buffer
	vk::CommandBufferAllocateInfo allocateInfo(
		pool,
		vk::CommandBufferLevel::ePrimary,
		1
	);

	vk::CommandBuffer commandBuffer;
	device.allocateCommandBuffers(&allocateInfo, &commandBuffer);

	// begin recording command buffer
	// we're only going to use the buffer once nad wait with returning from the function or until the copy has finished
	// we'll inform the driver about our intent with OneTimeSubmit
	commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	commandBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));

	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	queue.submit(1, &submitInfo, nullptr);
	queue.waitIdle();

	device.freeCommandBuffers(pool, 1, &commandBuffer);
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
