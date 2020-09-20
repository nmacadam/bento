#pragma once
#include <vulkan/vulkan.hpp>

struct VulkanContext
{
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::Device device;
	vk::PhysicalDevice physicalDevice;

	vk::CommandPool commandPool;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::Queue queue;

	vk::Extent2D swapChainExtent;
	int swapChainImageCount;

	vk::Sampler sampler;
	vk::ImageView imageView;
};
