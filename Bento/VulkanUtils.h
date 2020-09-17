#pragma once
#include <vulkan/vulkan.hpp>
#include "SwapChainSupportDetails.h"
#include "BufferData.h"
#include "ImageData.h"

namespace VulkanUtils
{
	// creator functions
	//vk::UniqueDebugUtilsMessengerEXT createDebugUtilsMessenger(vk::UniqueInstance & instance);
	VkDebugUtilsMessengerEXT createDebugUtilsMessenger(vk::UniqueInstance & instance);

	BufferData createBuffer(vk::Device device, vk::DeviceSize size, vk::PhysicalDevice physicalDevice, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties);
	//BufferData createBuffer(VmaAllocator* allocator, vk::Device device, vk::DeviceSize size, vk::PhysicalDevice physicalDevice, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperties);

	ImageData createImage(vk::Device device, vk::PhysicalDevice physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
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
	void copyBufferToImage(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::Buffer srcBuffer, vk::Image dstImage, uint32_t width, uint32_t height);
	bool checkLayers(std::vector<char const *> const & layers, std::vector<vk::LayerProperties> const & properties);

	vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool pool);
	void endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Device device, vk::CommandPool pool, vk::Queue queue);

	void transitionImageLayout(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
}
