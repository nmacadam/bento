#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "../core/Window.h"
#include "Vertex.h"
#include "BufferData.h"
#include "QueueFamilyIndices.h"
#include "VulkanContext.h"
#include "Mesh.h"
#include "Primitives.h"
#include "ImageData.h"

namespace bento
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void initialize(Window* window);
		void drawFrame();
		void clean();

		void rebuildCommandBuffers();

		static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
			auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
		}

		VulkanContext context;

		MeshFactory meshFactory = MeshFactory(&context);

	private:
		const std::vector<Vertex> vertices = Cube::vertices;
		const std::vector<uint32_t> indices = Cube::indices;

		Window* window;

		vk::UniqueInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		vk::UniqueSurfaceKHR surface;

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
		vk::UniquePipeline graphicsPipeline;

		vk::UniqueDescriptorSetLayout descriptorSetLayout;
		vk::UniqueDescriptorPool descriptorPool;
		std::vector<vk::UniqueDescriptorSet> descriptorSets;

		vk::UniqueDescriptorSetLayout objectDescriptorSetLayout;
		vk::UniqueDescriptorPool objectDescriptorPool;
		std::vector<vk::UniqueDescriptorSet> objectDescriptorSets;

		vk::UniqueCommandPool commandPool;
		std::vector<vk::UniqueCommandBuffer> commandBuffers;

		std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
		std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
		std::vector<vk::UniqueFence> inFlightFences;
		std::vector<vk::Fence> imagesInFlight;

		BufferData vertexBufferData;
		BufferData indexBufferData;

		std::vector<BufferData> uniformBufferData;
		std::vector<BufferData> transformationBufferData;

		BufferData stagingBuffer;

		// bundle
		ImageData textureImage;
		vk::UniqueImageView textureImageView;
		vk::UniqueSampler textureSampler;

		ImageData depthImage;
		vk::UniqueImageView depthImageView;

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
		void createObjectDescriptorSetLayout();
		void createGraphicsPipeline();
		void createFramebuffers();
		void createCommandPool();
		// move this out
		void createDepthResources();
		void createTextureImage();
		void createTextureImageView();
		void createTextureSampler();

		void createVertexBuffer();
		void createIndexBuffer();
		void createUniformBuffers();
		void createDescriptorPool();
		void createObjectDescriptorPool();
		void createDescriptorSets();
		void createObjectDescriptorSets();
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

}
