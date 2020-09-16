#pragma once
#include <vulkan/vulkan.hpp>
#include "SwapChainSupportDetails.h"
#include "BufferData.h"

namespace VulkanUtils
{
	// creator functions
	//vk::UniqueDebugUtilsMessengerEXT createDebugUtilsMessenger(vk::UniqueInstance & instance);
	VkDebugUtilsMessengerEXT createDebugUtilsMessenger(vk::UniqueInstance & instance);

	BufferData createBuffer(vk::Device device, vk::DeviceSize size, vk::PhysicalDevice physicalDevice, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties);
	//BufferData createBuffer(VmaAllocator* allocator, vk::Device device, vk::DeviceSize size, vk::PhysicalDevice physicalDevice, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	//// callbacks
	//VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
	//	VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
	//	VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
	//	VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
	//	void * /*pUserData*/);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData
	);

	// info functions
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

	uint32_t findMemoryType(vk::PhysicalDeviceMemoryProperties const & memoryProperties, uint32_t typeFilter, vk::MemoryPropertyFlags requirementsMask);

	// misc.
	void copyBuffer(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	bool checkLayers(std::vector<char const *> const & layers, std::vector<vk::LayerProperties> const & properties);
}
