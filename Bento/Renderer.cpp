#include "Renderer.h"
#include <iostream>
#include "VulkanUtils.h"
#include <set>
#include <algorithm>
#include "Shader.h"
#include "UniformBufferObject.h"
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

//static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
//	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
//
//	return VK_FALSE;
//}

Renderer::Renderer()
{
}


Renderer::~Renderer()
{
}

void Renderer::initialize(Window *window)
{
	this->window = window;
	initalizeVulkan();

	//VulkanContext vc = { &instance.get(), &surface.get(), &device.get(), &physicalDevice, &commandPool.get(), &graphicsQueue }

	context = VulkanContext{
		instance.get(),
		surface.get(),
		device.get(),
		physicalDevice,
		commandPool.get(),
		descriptorPool.get(),
		descriptorSetLayout.get(),
		graphicsQueue,
		swapChainExtent,
		static_cast<int>(swapChainImages.size()),

		textureSampler.get(),
		textureImageView.get()
	};
}

void Renderer::drawFrame()
{
	// wait for the frame to be finished
	device->waitForFences(inFlightFences[currentFrame].get(), true, UINT64_MAX);

	// get the index of the next available swap chain image
	uint32_t imageIndex;
	vk::Result acquireImageResult = device->acquireNextImageKHR(swapChain.get(), UINT64_MAX, imageAvailableSemaphores[currentFrame].get(), nullptr, &imageIndex);

	// check if swapchain needs to be rebuilt
	if (acquireImageResult == vk::Result::eErrorOutOfDateKHR)// || framebufferResized) // could cause a semaphore issue since it isnt after presentKHR
	{
		//framebufferResized = false;
		recreateSwapChain();
		return;
	}
	else if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != nullptr)
	{
		device->waitForFences(imagesInFlight[imageIndex], true, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame].get();

	updateUniformBuffer(imageIndex);

	for (size_t j = 0; j < meshFactory.count(); j++)
	{
		meshFactory.getMesh(j)->updateUniformBuffer(&context, imageIndex);
	}

	// gather requirements for submit info
	std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[currentFrame].get() };
	std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	std::array<vk::Semaphore, 1> signalSemaphores = { renderFinishedSemaphores[currentFrame].get() };

	vk::SubmitInfo submitInfo(
		waitSemaphores,
		waitStages,
		commandBuffers[imageIndex].get(),
		signalSemaphores
	);

	std::array<vk::SubmitInfo, 1> submits = { submitInfo };

	device->resetFences(inFlightFences[currentFrame].get());

	graphicsQueue.submit(submits, inFlightFences[currentFrame].get());

	// the last step of drawing a frame is submitting the result back to the swapchain to have it eventually show up on the screen
	// presentation is configured through a PresentInfoKHR
	std::array<vk::SwapchainKHR, 1> swapChains = { swapChain.get() };
	vk::PresentInfoKHR presentInfo(signalSemaphores, swapChains, imageIndex);

	// use standard vulkan present function since the other one throws an error about the
	// khr being out of date and automatically exits before i can rebuild the swapchain
	VkResult presentResult = vkQueuePresentKHR(presentQueue.operator VkQueue_T*(), &presentInfo.operator struct VkPresentInfoKHR&());
	//vk::Result presentResult = presentQueue.presentKHR(presentInfo);

	// check if swapchain needs to be rebuilt
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (presentResult != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	/*if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || framebufferResized)
	{
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (presentResult != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}*/

	presentQueue.waitIdle();

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::clean()
{
	device->waitIdle();

	// Unique references will automatically be deallocated

	meshFactory.clean();

	VulkanUtils::DestroyDebugUtilsMessengerEXT(instance->operator VkInstance_T*(), debugMessenger, nullptr);
}

void Renderer::initalizeVulkan()
{
	std::cout << "[INFO] : " << "Initializing Vulkan..." << std::endl;

	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createObjectDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createObjectDescriptorPool();
	createDescriptorSets();
	createObjectDescriptorSets();
	createCommandBuffers();
	createSyncObjects();

	std::cout << "[INFO] : " << "Vulkan initialized" << std::endl;
}

void Renderer::cleanupSwapChain()
{
	// release unique pointers so each element can be recreated afterwards
	device->destroyImageView(depthImageView.release());
	device->destroyImage(depthImage.image.release());
	device->freeMemory(depthImage.memory.release());

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		device->destroyFramebuffer(swapChainFramebuffers[i].release());
	}
	swapChainFramebuffers.clear();

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		device->freeCommandBuffers(commandPool.get(), commandBuffers[i].release());
	}
	commandBuffers.clear();

	device->destroyPipeline(graphicsPipeline.release());
	device->destroyPipelineLayout(pipelineLayout.release());
	device->destroyRenderPass(renderPass.release());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		device->destroyImageView(swapChainImageViews[i].release(), nullptr);\
	}
	swapChainImageViews.clear();

	device->destroySwapchainKHR(swapChain.release());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		device->destroyBuffer(uniformBufferData[i].buffer.release());
		device->freeMemory(uniformBufferData[i].memory.release());
	}
	uniformBufferData.clear();

	device->destroyDescriptorPool(descriptorPool.release());
}

void Renderer::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window->getHandle(), &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window->getHandle(), &width, &height);
		glfwWaitEvents();
	}

	device->waitIdle();

	std::cout << "[INFO] : " << "Recreating the swap chain" << std::endl;

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void Renderer::createInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport()) 
	{
		std::cout << "Set the environment variable VK_LAYER_PATH to point to the location of your layers" << std::endl;

		throw std::runtime_error("validation layers requested, but not available!");
	}

	vk::ApplicationInfo applicationInfo("bento test", 1, "bento", 1, VK_API_VERSION_1_1);

	auto extensions = getRequiredExtensions();
	
	vk::InstanceCreateInfo instanceCreateInfo(
		vk::InstanceCreateFlags(),
		&applicationInfo,
		static_cast<uint32_t>(instanceLayerNames.size()),
		instanceLayerNames.data(),
		static_cast<uint32_t>(extensions.size()),
		extensions.data()
	);

	/*if (enableValidationLayers) 
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayerNames.size());
		instanceCreateInfo.ppEnabledLayerNames = instanceLayerNames.data();
	}
	else 
	{
		instanceCreateInfo.enabledLayerCount = 0;
	}*/

	//std::cout << glfwExtensionCount << std::endl;

	instance = vk::createInstanceUnique(instanceCreateInfo);

	std::cout << "[INFO] : " << "Created Vulkan instance" << std::endl;
}

void Renderer::setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	debugMessenger = VulkanUtils::createDebugUtilsMessenger(instance);

	std::cout << "[INFO] : " << "Created debug messenger" << std::endl;
}

void Renderer::createSurface()
{
	// create a surface
	{
		VkSurfaceKHR _surface;
		glfwCreateWindowSurface(VkInstance(instance.get()), window->getHandle(), nullptr, &_surface);
		vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(instance.get());
		surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface), _deleter);
	}

	std::cout << "[INFO] : " << "Created surface" << std::endl;
}

void Renderer::pickPhysicalDevice()
{
	// select a graphics card in the system that supports the features we want
	// we'll stick to the first graphics card that supports our needs for now

	// get a list of physical devices
	std::vector<vk::PhysicalDevice> devices = instance->enumeratePhysicalDevices();

	// find a device that is suitable
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == nullptr) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	// get the device's name for an info log
	vk::PhysicalDeviceProperties properties;
	physicalDevice.getProperties(&properties);

	std::cout << "[INFO] : " << "Selected physical device " << properties.deviceName << std::endl;
}

void Renderer::createLogicalDevice()
{
	// the logical device is an object we can work with the corresponds to the physical device we chose

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	// provide info about the queues we want to create for the device
	// - queue priority allows vulkan to schedule command buffer execution by priority; this is required
	// - the create info describes the number of queues we want for a single queue family
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo(
			{},
			queueFamily,
			1,
			&queuePriority
		);

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Request specific device features
	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.samplerAnisotropy = true;

	// get a list of extensions to enable for the device
	std::vector<char const *> enabledExtensions;
	enabledExtensions.reserve(deviceExtensions.size());
	for (auto const & ext : deviceExtensions)
	{
		enabledExtensions.push_back(ext.data());
	}

	// put together our create info
	vk::DeviceCreateInfo deviceCreateInfo(
		vk::DeviceCreateFlags(),
		queueCreateInfos,
		{},
		enabledExtensions,
		&deviceFeatures
	);

	// create UniqueDevice
	device = physicalDevice.createDeviceUnique(deviceCreateInfo);

	// retrieve queue handle for the graphics queue and present queue
	device->getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
	device->getQueue(indices.presentFamily.value(), 0, &presentQueue);

	std::cout << "[INFO] : " << "Created logical device" << std::endl;
}

void Renderer::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = VulkanUtils::querySwapChainSupport(physicalDevice, surface.get());

	// retrieve swap chain details
	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	// how many images (minimum) do we want in the swap chain? we'll add 1 to that so there's no waiting on the driver to retrieve a new image
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// make sure we don't exceed the maximum number of images while doing this
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	// partially complete create info
	vk::SwapchainCreateInfoKHR createInfo(
		{},
		surface.get(),
		imageCount,
		surfaceFormat.format,
		surfaceFormat.colorSpace,
		extent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment
	);

	// specify how to handle swap chain images that will be used across multiple queue families
	// are the shared? or exclusive to one family?
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	// pretransform allows us to apply transformations to images before use
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	// specifies if the alpha channel should be used for blending (we'll ignore it for now)
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	// don't care if pixels are obscured by another window or something
	createInfo.clipped = VK_TRUE;
	// old swap chain for if the the swap chain has to be rebuilt
	createInfo.oldSwapchain = nullptr;

	// create swap chain
	swapChain = device->createSwapchainKHRUnique(createInfo);

	// get images from swap chain
	swapChainImages = device->getSwapchainImagesKHR(swapChain.get());

	// store format and extent for when we make the image views
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	std::cout << "[INFO] : " << "Created swap chain" << std::endl;
}

// create a new image view for each swap chain image
void Renderer::createImageViews()
{
	// to use an image, including those in the swap chain, the render pipeline requires a
	// imageView object.  it describes how to access the image and which part to access

	swapChainImageViews.reserve(swapChainImages.size());

	// create a new image view for each swap chain image
	for (const auto swapChainImage : swapChainImages)
	{
		swapChainImageViews.push_back(VulkanUtils::createImageView(device.get(), swapChainImage, swapChainImageFormat, vk::ImageAspectFlagBits::eColor));
	}

	std::cout << "[INFO] : " << "Created swap chain image views" << std::endl;
}

void Renderer::createRenderPass()
{
	// the render pass details how many color and depth buffers there are,
	// how many samples to use for each of them, and how their contents should be handled throughout the rendering process

	// we'll have a single color buffer attachment represented by one of the images from the swap chain
	vk::AttachmentDescription colorAttachment(
		{},
		swapChainImageFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	);

	// depth attachment for the image
	vk::AttachmentDescription depthAttachment(
		{},
		VulkanUtils::findDepthFormat(physicalDevice),
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	// a single renderpass can consist of multiple subpasses
	// subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes (ex: post-processing a previous pass)
	// by grouping these rendering operations into a a single pass, Vulkan is able to reorder the operations and conserve memory bandwidth

	// a reference to the color attachment
	vk::AttachmentReference colorAttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

	// a reference to the depth attachment
	vk::AttachmentReference depthAttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	// describe the subpass; where does it exist in the pipeline; what are its attachments
	vk::SubpassDescription subpass(
		{},
		vk::PipelineBindPoint::eGraphics,
		{},
		{},
		1,
		&colorAttachmentReference
	);
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	// iffy about this one
	// we need a subpass dependency to automatically take care of image layout transitions
	vk::SubpassDependency dependency(
		VK_SUBPASS_EXTERNAL,
		0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		static_cast<vk::AccessFlagBits>(0),
		vk::AccessFlagBits::eColorAttachmentWrite
	);

	// create render pass
	std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	vk::RenderPassCreateInfo renderPassInfo(
		{},
		attachments.size(),
		attachments.data(),
		1,
		&subpass,
		1,
		&dependency
	);
	renderPass = device->createRenderPassUnique(renderPassInfo);

	std::cout << "[INFO] : " << "Created render pass" << std::endl;
}

void Renderer::createDescriptorSetLayout()
{
	// transferring frame-updated information to the gpu can be slow if not done correctly
	// we'll be using a resource descriptor; its how we will access our uniform buffer object

	// describe layout binding for uniform buffer
	vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1);
	uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	// describe layout for model matrix uniform variable
	vk::DescriptorSetLayoutBinding modelMatBinding(1, vk::DescriptorType::eUniformBuffer, 1);
	modelMatBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	modelMatBinding.pImmutableSamplers = nullptr;

	// describe layout binding for sampler
	vk::DescriptorSetLayoutBinding samplerLayoutBinding(
		2,
		vk::DescriptorType::eCombinedImageSampler,
		1,
		vk::ShaderStageFlagBits::eFragment,
		nullptr
	);

	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, modelMatBinding, samplerLayoutBinding };

	// create descriptor set for ubo
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings.size(), bindings.data());
	descriptorSetLayout = device->createDescriptorSetLayoutUnique(layoutInfo);

	std::cout << "[INFO] : " << "Created descriptor set layout" << std::endl;
}

void Renderer::createObjectDescriptorSetLayout()
{
	// transferring frame-updated information to the gpu can be slow if not done correctly
	// we'll be using a resource descriptor; its how we will access our uniform buffer object

	// describe layout binding for uniform buffer
	vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1);
	uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	// describe layout binding for sampler
	vk::DescriptorSetLayoutBinding samplerLayoutBinding(
		1,
		vk::DescriptorType::eCombinedImageSampler,
		1,
		vk::ShaderStageFlagBits::eFragment,
		nullptr
	);

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	// create descriptor set for ubo
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings.size(), bindings.data());
	objectDescriptorSetLayout = device->createDescriptorSetLayoutUnique(layoutInfo);

	std::cout << "[INFO] : " << "Created object descriptor set layout" << std::endl;
}

void Renderer::createGraphicsPipeline()
{
	// we now need to set up and configure a graphics pipeline for drawing an image
	// this is effectively the process for getting stuff to the shaders
	Shader vertexShader(device, "shaders/vert.spv");
	Shader fragmentShader(device, "shaders/frag.spv");

	// assign the shaders to a specific pipeline stage
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eVertex,
		vertexShader.shaderModule.get(),
		"main"
	);

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eFragment,
		fragmentShader.shaderModule.get(),
		"main"
	);

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	// describe the format of the vertex data
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
		{},
		1,
		&bindingDescription,
		attributeDescriptions.size(),
		attributeDescriptions.data()
	);

	// describe the kind of geometry being drawn and if primitive restart should be enabled
	// primitive restart allows us to use an element buffer
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
		{},
		vk::PrimitiveTopology::eTriangleList,
		false
	);

	// describe the region of the framebuffer that the output will be rendered to (in our case, the whole window)
	vk::Viewport viewport(
		0.0f,
		0.0f,
		static_cast<float>(swapChainExtent.width),
		static_cast<float>(swapChainExtent.height),
		0.0f,
		1.0f
	);

	// scissor allows us to cut out parts of the viewport area, we want all of it though
	vk::Rect2D scissor(
		{ 0, 0 },
		swapChainExtent
	);

	// combine the viewport and scissor into viewport state
	vk::PipelineViewportStateCreateInfo viewportState(
		{},
		1,
		&viewport,
		1,
		&scissor
	);

	// describe rasterization process; depth testing, face culling, polygon types, etc.
	vk::PipelineRasterizationStateCreateInfo rasterizer(
		{},
		false,
		false,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		false,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	);

	// configure multisampling (requires a gpu feature)
	// we'll keep it disabled for now
	vk::PipelineMultisampleStateCreateInfo multisampling(
		{},
		vk::SampleCountFlagBits::e1,
		false,
		1.0f,
		nullptr,
		false,
		false
	);

	// depth and stencil testing
	// - depth comare op: specifies the comparison that is performed to keep or discard fragments
	//		we're sticking to lower depth = closer
	// - not using stencil for now
	vk::PipelineDepthStencilStateCreateInfo depthStencil(
		{},
		true,
		true,
		vk::CompareOp::eLess,
		false,
		false,
		{},
		{},
		0.0f,
		1.0f
	);

	// describe color blend configuration per attached framebuffer
	vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		false,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);

	// describe global color blending settings
	vk::PipelineColorBlendStateCreateInfo colorBlending(
		{},
		false,
		vk::LogicOp::eCopy,
		1,
		&colorBlendAttachment
		// blend constants
	);

	// dynamic state stuff goes here
	// using a dynamic state means data needs to be specified at draw time
	// https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

	std::array<vk::DescriptorSetLayout, 2> desriptorSetLayouts = {
		descriptorSetLayout.get(),
		objectDescriptorSetLayout.get()
	};

	// create the graphics pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo(	// check here later
		{},
		2,
		desriptorSetLayouts.data(),
		0,
		nullptr
	);
	pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutInfo);

	// create the graphics pipeline
	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
		{},									// flags
		shaderStages,								// stages
		&vertexInputInfo,							// pVertexInputState
		&inputAssembly,								// pInputAssemblyState
		nullptr,                   // pTessellationState
		&viewportState,								// pViewportState
		&rasterizer,								// pRasterizationState
		&multisampling,								// pMultisampleState
		&depthStencil,								// pDepthStencilState
		&colorBlending,								// pColorBlendState
		nullptr,						// pDynamicState
		pipelineLayout.get(),					// layout
		renderPass.get()					// renderPass
	);
	graphicsPipeline = device->createGraphicsPipelineUnique(nullptr, graphicsPipelineCreateInfo);

	std::cout << "[INFO] : " << "Created graphics pipeline" << std::endl;
}

void Renderer::createFramebuffers()
{
	// the framebuffer references all of the image views that represent the attachments
	// we need to create a framebuffer for all of the images in the swapchain and use the one that  corresponds to the retrieved image at draw time
	swapChainFramebuffers.reserve(swapChainImageViews.size());
	for (auto const & view : swapChainImageViews)
	{
		// bundle image views for the framebuffer
		std::array<vk::ImageView, 2> attachments = { view.get(), depthImageView.get() };

		vk::FramebufferCreateInfo framebufferInfo(
			vk::FramebufferCreateFlags(),
			renderPass.get(),
			attachments.size(),
			attachments.data(),
			swapChainExtent.width,
			swapChainExtent.height,
			1
		);

		swapChainFramebuffers.push_back(device->createFramebufferUnique(framebufferInfo));
	}

	std::cout << "[INFO] : " << "Created frame buffers" << std::endl;
}

void Renderer::createCommandPool()
{
	// command pools manage the memory that is used ot store the buffers and command buffers are allocated from them
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	commandPool = device->createCommandPoolUnique(
		vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), queueFamilyIndices.graphicsFamily.value()));

	std::cout << "[INFO] : " << "Created command pool" << std::endl;
}

void Renderer::createDepthResources()
{
	// get the depth image format
	vk::Format depthFormat = VulkanUtils::findDepthFormat(physicalDevice);

	// create the depth image
	depthImage = VulkanUtils::createImage(
		device.get(),
		physicalDevice,
		swapChainExtent.width,
		swapChainExtent.height,
		depthFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	// create the depth image view
	depthImageView = VulkanUtils::createImageView(device.get(), depthImage.image.get(), depthFormat, vk::ImageAspectFlagBits::eDepth);

	std::cout << "[INFO] : " << "Created depth resources" << std::endl;
}

void Renderer::createTextureImage()
{
	// retrieve image pixels with stbi
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// calculate image size (x4 for the rgba)
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	// create a staging buffer for the image pixels so we can then transfer
	stagingBuffer = VulkanUtils::createBuffer(
		device.get(),
		imageSize,
		physicalDevice,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);

	// copy the pixel data to the staging buffer
	void* data = device->mapMemory(stagingBuffer.memory.get(), 0, imageSize, {});
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	device->unmapMemory(stagingBuffer.memory.get());

	// free image pixel memory
	stbi_image_free(pixels);

	// create the image
	textureImage = VulkanUtils::createImage(
		device.get(),
		physicalDevice, 
		texWidth,
		texHeight, 
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	// prepare the image to be copied (using transfer destination optimal)
	VulkanUtils::transitionImageLayout(
		device.get(), commandPool.get(), graphicsQueue, 
		textureImage.image.get(),
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageLayout::eUndefined, 
		vk::ImageLayout::eTransferDstOptimal
	);

	// copy staging buffer contents to image
	VulkanUtils::copyBufferToImage(
		device.get(), commandPool.get(), graphicsQueue,
		stagingBuffer.buffer.get(),
		textureImage.image.get(),
		texWidth,
		texHeight
	);

	// transition layout to prepare for shader access
	VulkanUtils::transitionImageLayout(
		device.get(), commandPool.get(), graphicsQueue,
		textureImage.image.get(),
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal
	);

	std::cout << "[INFO] : " << "Created texture image" << std::endl;
}

void Renderer::createTextureImageView()
{
	textureImageView = VulkanUtils::createImageView(device.get(), textureImage.image.get(), vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

	std::cout << "[INFO] : " << "Created texture image view" << std::endl;
}

void Renderer::createTextureSampler()
{
	// specify sampler info
	// - mag/min filters specify how to interpolate texels that are magnified or minified
	// - mipmap mode
	// - address mode defines how to read the texture past the image dimensions (i.e. repeat, clamp, etc.)
	// - mipmap lod bias
	// - anisotropy enable specifies whether to use anisotropic filtering; no reason not to use this unless performance is a concern
	// - max anisotropy limits the amount of texel samples used to calculate the final result
	// - compare operation is used for filtering; texels will first be compared to a value, and the result is used in the filtering operation
	// - min/max lod 
	// - border color is the color returned when sampling beyond the image with clamp to border addressing
	// - unnormalized coordinates; should coordinates not be in the [0, 1) range? they probably should
	vk::SamplerCreateInfo samplerInfo(
		{},
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		0.0f,
		true, 16.0f,
		false, vk::CompareOp::eAlways,
		0.0f, 0.0f,
		vk::BorderColor::eIntOpaqueBlack,
		false
	);

	textureSampler = device->createSamplerUnique(samplerInfo);

	std::cout << "[INFO] : " << "Created texture sampler" << std::endl;
}

void Renderer::createVertexBuffer()
{
	// create the staging buffer
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	BufferData stagingBufferData = VulkanUtils::createBuffer(
		device.get(),
		bufferSize,
		physicalDevice,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);


	// copy the vertex data to the staging buffer
	uint8_t* data = static_cast<uint8_t*>(device->mapMemory(stagingBufferData.memory.get(), 0, bufferSize, {}));
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	device->unmapMemory(stagingBufferData.memory.get());

	// create the vertex buffer
	vertexBufferData = VulkanUtils::createBuffer(
		device.get(),
		bufferSize,
		physicalDevice,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	// copy data from staging buffer to vertex buffer
	VulkanUtils::copyBuffer(
		device.get(),
		commandPool.get(),
		graphicsQueue,
		stagingBufferData.buffer.get(),
		vertexBufferData.buffer.get(), bufferSize
	);

	std::cout << "[INFO] : " << "Created vertex buffer" << std::endl;
}

void Renderer::createIndexBuffer()
{
	// create the staging buffer
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	BufferData stagingBufferData = VulkanUtils::createBuffer(
		device.get(),
		bufferSize,
		physicalDevice,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);


	// copy the vertex data to the staging buffer
	uint8_t* data = static_cast<uint8_t*>(device->mapMemory(stagingBufferData.memory.get(), 0, bufferSize, {}));
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	device->unmapMemory(stagingBufferData.memory.get());

	// create the vertex buffer
	indexBufferData = VulkanUtils::createBuffer(
		device.get(),
		bufferSize,
		physicalDevice,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	// copy data from staging buffer to vertex buffer
	VulkanUtils::copyBuffer(
		device.get(),
		commandPool.get(),
		graphicsQueue,
		stagingBufferData.buffer.get(),
		indexBufferData.buffer.get(), bufferSize
	);

	std::cout << "[INFO] : " << "Created index buffer" << std::endl;
}

void Renderer::createUniformBuffers()
{
	vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);
	uniformBufferData.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		uniformBufferData[i] = VulkanUtils::createBuffer(
			device.get(),
			uniformBufferSize,
			physicalDevice,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
	}

	vk::DeviceSize transformBufferSize = sizeof(TransformationUBO);
	transformationBufferData.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		transformationBufferData[i] = VulkanUtils::createBuffer(
			device.get(),
			transformBufferSize,
			physicalDevice,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
	}

	std::cout << "[INFO] : " << "Created uniform buffers" << std::endl;
}

void Renderer::createDescriptorPool()
{
	// set pool sizes for each descriptor
	std::array<vk::DescriptorPoolSize, 2> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, swapChainImages.size()),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, swapChainImages.size()),
		//vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, swapChainImages.size())
	};
	std::cout << "[WARNING] : " << "Arbitrarily changing set count for descriptor pool" << std::endl;
	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, swapChainImages.size() * 10, 1, poolSizes.data());

	descriptorPool = device->createDescriptorPoolUnique(poolInfo);

	std::cout << "[INFO] : " << "Created descriptor pool" << std::endl;
}

void Renderer::createObjectDescriptorPool()
{
	// set pool sizes for each descriptor
	std::array<vk::DescriptorPoolSize, 2> poolSizes = {
		//vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, swapChainImages.size()),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, swapChainImages.size()),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, swapChainImages.size())
	};
	std::cout << "[WARNING] : " << "Arbitrarily changing set count for descriptor pool" << std::endl;
	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, swapChainImages.size() * 10, 1, poolSizes.data());

	objectDescriptorPool = device->createDescriptorPoolUnique(poolInfo);

	std::cout << "[INFO] : " << "Created object descriptor pool" << std::endl;
}

void Renderer::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout.get());

	vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool.get(), swapChainImages.size(), layouts.data());

	descriptorSets.resize(swapChainImages.size());
	descriptorSets = device->allocateDescriptorSetsUnique(allocateInfo);

	// populate descriptors
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		vk::DescriptorBufferInfo uniformBufferInfo(uniformBufferData[i].buffer.get(), 0, sizeof(UniformBufferObject));
		//vk::DescriptorBufferInfo modelMatInfo(transformationBufferData[i].buffer.get(), 0, sizeof(TransformationUBO));
		//vk::DescriptorImageInfo imageInfo(textureSampler.get(), textureImageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal);

		std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
			vk::WriteDescriptorSet(
				descriptorSets[i].get(),
				0,
				0, 
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&uniformBufferInfo,
				nullptr
			)/*,
			vk::WriteDescriptorSet(
				descriptorSets[i].get(),
				1,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&modelMatInfo,
				nullptr
			),
			vk::WriteDescriptorSet(
				descriptorSets[i].get(),
				2,
				0, 
				1,
				vk::DescriptorType::eCombinedImageSampler,
				&imageInfo,
				nullptr,
				nullptr
			)*/
		};

		device->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	std::cout << "[INFO] : " << "Created descriptor set" << std::endl;
}

void Renderer::createObjectDescriptorSets()
{
	/*{
		std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout.get());

		vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool.get(), swapChainImages.size(), layouts.data());

		descriptorSets.resize(swapChainImages.size());
		descriptorSets = device->allocateDescriptorSetsUnique(allocateInfo);
	}*/

	{
		std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), objectDescriptorSetLayout.get());

		vk::DescriptorSetAllocateInfo allocateInfo(objectDescriptorPool.get(), swapChainImages.size(), layouts.data());

		objectDescriptorSets.resize(swapChainImages.size());
		objectDescriptorSets = device->allocateDescriptorSetsUnique(allocateInfo);
	}

	// populate descriptors
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		//vk::DescriptorBufferInfo uniformBufferInfo(uniformBufferData[i].buffer.get(), 0, sizeof(UniformBufferObject));
		vk::DescriptorBufferInfo modelMatInfo(transformationBufferData[i].buffer.get(), 0, sizeof(TransformationUBO));
		vk::DescriptorImageInfo imageInfo(textureSampler.get(), textureImageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal);

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
			/*vk::WriteDescriptorSet(
				descriptorSets[i].get(),
				0,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&uniformBufferInfo,
				nullptr
			),*/
			vk::WriteDescriptorSet(
				objectDescriptorSets[i].get(),
				0,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&modelMatInfo,
				nullptr
			),
			vk::WriteDescriptorSet(
				objectDescriptorSets[i].get(),
				1,
				0,
				1,
				vk::DescriptorType::eCombinedImageSampler,
				&imageInfo,
				nullptr,
				nullptr
			)
		};

		device->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	std::cout << "[INFO] : " << "Created object descriptor set" << std::endl;
}

void Renderer::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	// create command buffers
	vk::CommandBufferAllocateInfo allocateInfo(
		commandPool.get(),
		vk::CommandBufferLevel::ePrimary,
		commandBuffers.size()
	);
	commandBuffers = device->allocateCommandBuffersUnique(allocateInfo);

	// start recording commands for each command buffer
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		// none of the begin info flags are applicable for us right now
		vk::CommandBufferBeginInfo beginInfo(
			{},
			nullptr
		);

		commandBuffers[i]->begin(beginInfo);

		// configure render pass; include the pass itself and the attachments to bind
		//renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		//renderPassInfo.renderArea.extent = swapChainExtent;

		// include clear values for the color and depth image
		std::array<vk::ClearValue, 2> clearValues = {
			vk::ClearValue(vk::ClearColorValue(std::array<uint32_t, 4>{0, 0, 0, 1})),
			vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))
		};

		//renderPassInfo.clearValueCount = clearValues.size();
		//renderPassInfo.pClearValues = clearValues.data();

		if (swapChainFramebuffers[i].get() == nullptr)
		{
			std::cerr << "framebuffer is null!" << std::endl;
		}

		vk::RenderPassBeginInfo renderPassInfo(
			renderPass.get(),
			swapChainFramebuffers[i].get(), 
			vk::Rect2D(vk::Offset2D(0,0), swapChainExtent), 
			clearValues.size(),
			clearValues.data()
		);

		// begin the render pass
		commandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// bind the graphics pipeline
		commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

		//for (size_t j = 0; j < meshFactory.count(); j++)
		//{
		//	std::cout << "adding mesh" << std::endl;

		//	// bind vertex buffers
		//	commandBuffers[i]->bindVertexBuffers(0, meshFactory.getMesh(j)->getVertexBufferData(), { 0 });
		//	commandBuffers[i]->bindIndexBuffer(meshFactory.getMesh(j)->getIndexBufferData(), 0, vk::IndexType::eUint32);

		//	// bind descriptor sets
		//	commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, descriptorSets[i].get(), nullptr);

		//	// draw
		//	commandBuffers[i]->drawIndexed(static_cast<uint32_t>(meshFactory.getMesh(j)->indices.size()), 1, 0, 0, 0);
		//}

		// bind vertex buffers
		std::array<vk::Buffer, 1> vertexBuffers = { vertexBufferData.buffer.get() };
		std::array<vk::DeviceSize, 1> offsets = { 0 };
		commandBuffers[i]->bindVertexBuffers(0, vertexBuffers, offsets);
		commandBuffers[i]->bindIndexBuffer(indexBufferData.buffer.get(), 0, vk::IndexType::eUint32);

		// bind descriptor sets
		std::array<vk::DescriptorSet, 2> bindDescriptorSets;
		bindDescriptorSets[0] = descriptorSets[i].get();
		bindDescriptorSets[1] = objectDescriptorSets[i].get();

		commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, bindDescriptorSets, nullptr);

		// draw
		commandBuffers[i]->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		// end the render pass
		commandBuffers[i]->endRenderPass();

		// finish recording
		commandBuffers[i]->end();
	}

	std::cout << "[INFO] : " << "Created command buffers" << std::endl;
}

void Renderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size());

	// create semaphore for each frame
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		imageAvailableSemaphores[i] = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		renderFinishedSemaphores[i] = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		// create already signaled so it works on the first frame
		inFlightFences[i] = device->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}

	std::cout << "[INFO] : " << "Created sync objects" << std::endl;
}

void Renderer::rebuildCommandBuffers()
{
	// start recording commands for each command buffer
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		// none of the begin info flags are applicable for us right now
		vk::CommandBufferBeginInfo beginInfo(
			{},
			nullptr
		);

		commandBuffers[i]->begin(beginInfo);

		// configure render pass; include the pass itself and the attachments to bind
		//renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		//renderPassInfo.renderArea.extent = swapChainExtent;

		// include clear values for the color and depth image
		std::array<vk::ClearValue, 2> clearValues = {
			vk::ClearValue(vk::ClearColorValue(std::array<uint32_t, 4>{0, 0, 0, 1})),
			vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))
		};

		//renderPassInfo.clearValueCount = clearValues.size();
		//renderPassInfo.pClearValues = clearValues.data();

		if (swapChainFramebuffers[i].get() == nullptr)
		{
			std::cerr << "framebuffer is null!" << std::endl;
		}

		vk::RenderPassBeginInfo renderPassInfo(
			renderPass.get(),
			swapChainFramebuffers[i].get(),
			vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent),
			clearValues.size(),
			clearValues.data()
		);

		// begin the render pass
		commandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// bind the graphics pipeline
		commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

		for (size_t j = 0; j < meshFactory.count(); j++)
		{
			std::cout << "adding mesh" << std::endl;

			// bind vertex buffers
			commandBuffers[i]->bindVertexBuffers(0, meshFactory.getMesh(j)->getVertexBufferData(), { 0 });
			commandBuffers[i]->bindIndexBuffer(meshFactory.getMesh(j)->getIndexBufferData(), 0, vk::IndexType::eUint32);

			// bind descriptor sets
			commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, meshFactory.getMesh(j)->getDescriptorSet(i), nullptr);
			//commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, descriptorSets[i].get(), nullptr);

			// draw
			commandBuffers[i]->drawIndexed(static_cast<uint32_t>(meshFactory.getMesh(j)->indices.size()), 1, 0, 0, 0);
		}

		//// bind vertex buffers
		//std::array<vk::Buffer, 1> vertexBuffers = { vertexBufferData.buffer.get() };
		//std::array<vk::DeviceSize, 1> offsets = { 0 };
		//commandBuffers[i]->bindVertexBuffers(0, vertexBuffers, offsets);
		//commandBuffers[i]->bindIndexBuffer(indexBufferData.buffer.get(), 0, vk::IndexType::eUint32);

		//// bind descriptor sets
		//commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, descriptorSets[i].get(), nullptr);

		//// draw
		//commandBuffers[i]->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		// end the render pass
		commandBuffers[i]->endRenderPass();

		// finish recording
		commandBuffers[i]->end();
	}

	std::cout << "[INFO] : " << "Rebuilt command buffers" << std::endl;
}

void Renderer::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	{
		UniformBufferObject ubo{};
		//ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.0f));
		//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.5f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		void* data = device->mapMemory(uniformBufferData[currentImage].memory.get(), 0, sizeof(ubo), {});
		memcpy(data, &ubo, sizeof(ubo));
		device->unmapMemory(uniformBufferData[currentImage].memory.get());
	}

	{
		TransformationUBO ubo{};
		//ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.0f));
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.5f, 0.0f, 1.0f));

		void* data = device->mapMemory(transformationBufferData[currentImage].memory.get(), 0, sizeof(ubo), {});
		memcpy(data, &ubo, sizeof(ubo));
		device->unmapMemory(transformationBufferData[currentImage].memory.get());
	}
}

std::vector<const char*> Renderer::getRequiredExtensions()
{
	// get the extensions required by glfw
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// add validation layer extensions if active
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool Renderer::checkValidationLayerSupport()
{
	std::vector<vk::LayerProperties> instanceLayerProperties = vk::enumerateInstanceLayerProperties();

	// Use standard_validation meta layer that enables all recommended validation layers
	return VulkanUtils::checkLayers(instanceLayerNames, instanceLayerProperties);
}

bool Renderer::isDeviceSuitable(vk::PhysicalDevice device)
{
	// get queue family indices for the graphics and present queue for device
	QueueFamilyIndices indices = findQueueFamilies(device);

	// does the device support requested extensions
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	// does the device support requested swap chain features
	bool swapChainAdequate = false;
	if (extensionsSupported) 
	{
		SwapChainSupportDetails swapChainSupport = VulkanUtils::querySwapChainSupport(device, surface.get());
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	vk::PhysicalDeviceFeatures supportedFeatures;
	device.getFeatures(&supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool Renderer::checkDeviceExtensionSupport(vk::PhysicalDevice device)
{
	// enumerate extensions and check if we have all the required extensions
	std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties(nullptr);

	// check that all required extensions are supported
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	// check if our preferred format combination is available
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	// otherwise just settle for whatever is first (or take the time to rank what is available, you know, later)
	return availableFormats[0];
}

// choose present mode; represents the conditions for showing images to the screen; defaults to FIFO
vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	// check if our preferred format combination is available
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	// otherwise just settle a guaranteed mode
	return vk::PresentModeKHR::eFifo;
}

// choose the swap extent; more than likely will be the window resolution
vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	// the swap extent is the resolution of the swap chain images
	// we want to match the resolution of the window

	// some window managers allow setting the extent to match UINT32_MAX for special purposes
	if (capabilities.currentExtent.width != UINT32_MAX) 
	{
		return capabilities.currentExtent;
	}
	else // otherwise get the best within the minImageExtent and maxImageExtent bounds
	{
		VkExtent2D actualExtent = { static_cast<uint32_t>(window->getWidth()), static_cast<uint32_t>(window->getHeight()) };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

QueueFamilyIndices Renderer::findQueueFamilies(vk::PhysicalDevice device)
{
	// almost every operation in vulkan must be submitted to a queue
	// there are different types of queues that originate from different queue families
	// and each family only allows a certain subset of commands
	// we need to check which queue families are supported by the device and which
	// one of these supports the commands we want to use

	QueueFamilyIndices indices;

	// get the QueueFamilyProperties of the PhysicalDevice
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilyProperties) {
		// check for whether the queue has graphics capabilities
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		// check if the queue family can present to our window surface
		vk::Bool32 presentSupport = false;
		device.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface.get(), &presentSupport);
		
		if (presentSupport) {
			indices.presentFamily = i;
		}

		// if both requirements are met, break
		if (indices.isComplete()) {
			break;
		}

		i++;
	}

    return indices;
}
