#pragma once

#include <vulkan/vulkan.hpp>
//#include <vk_mem_alloc.h>
#include <iostream>
#include "QueueFamilyIndices.h"
#include "Window.h"
#include "SwapChainSupportDetails.h"
#include "Vertex.h"
#include "BufferData.h"
#include "ImageData.h"

class Renderer
{
public:
	Renderer();
	~Renderer();

	void initialize(Window* window);
	void drawFrame();
	void clean();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

private:
	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};
	const std::vector<uint32_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	Window* window;

	vk::UniqueInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	vk::UniqueSurfaceKHR surface;

	//VmaAllocator allocator;

	vk::PhysicalDevice physicalDevice;
	vk::UniqueDevice device;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	vk::UniqueSwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::UniqueImageView> swapChainImageViews;
	std::vector<vk::UniqueFramebuffer> swapChainFramebuffers;

	vk::UniqueRenderPass renderPass;
	vk::UniquePipelineLayout pipelineLayout;
	vk::UniqueDescriptorSetLayout descriptorSetLayout;
	vk::UniquePipeline graphicsPipeline;

	vk::UniqueDescriptorPool descriptorPool;
	std::vector<vk::UniqueDescriptorSet> descriptorSets;

	vk::UniqueCommandPool commandPool;
	std::vector<vk::UniqueCommandBuffer> commandBuffers;

	std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
	std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
	std::vector<vk::UniqueFence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;

	BufferData vertexBufferData;
	BufferData indexBufferData;

	std::vector<BufferData> uniformBufferData;

	BufferData stagingBuffer;

	// bundle
	ImageData textureImage;

	const int MAX_FRAMES_IN_FLIGHT = 2;
	size_t currentFrame = 0;

	bool framebufferResized = false;

	const std::vector<const char*> instanceLayerNames = {
		"VK_LAYER_KHRONOS_validation"
	};

	//const std::vector<const char*> deviceExtensions = {
	const std::vector<std::string> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void initalizeVulkan();

	void cleanupSwapChain();
	void recreateSwapChain();

	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	//void createMemoryAllocator();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	// move this out
	void createTextureImage();
	
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();

	void updateUniformBuffer(uint32_t currentImage);

	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	bool isDeviceSuitable(vk::PhysicalDevice device);
	bool checkDeviceExtensionSupport(vk::PhysicalDevice device);

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
};

