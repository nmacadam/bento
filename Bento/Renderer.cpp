#include "Renderer.h"
#include <iostream>
#include "VulkanUtils.h"
#include <set>
#include <algorithm>
#include "Shader.h"

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
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createCommandBuffers();
	createSyncObjects();

	std::cout << "[INFO] : " << "Vulkan initialized" << std::endl;
}

void Renderer::cleanupSwapChain()
{
	// release unique pointers so each element can be recreated afterwards
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
		device->destroyImageView(swapChainImageViews[i].release(), nullptr);
	}
	swapChainImageViews.clear();

	device->destroySwapchainKHR(swapChain.release());
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
	createFramebuffers();
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
		{}
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

	// apply a standard component mapping (for swizzling)
	vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
	// subresource range describes the image's purpose and which parts to access
	// our image is a color target without mipmapping or multiple layers (for now)
	vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

	// create a new image view for each swap chain image
	for (const auto swapChainImage : swapChainImages)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo(
			vk::ImageViewCreateFlags(),
			swapChainImage,
			vk::ImageViewType::e2D,
			swapChainImageFormat, 
			componentMapping, 
			subResourceRange
		);
		swapChainImageViews.push_back(device->createImageViewUnique(imageViewCreateInfo));
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

	// a single renderpass can consist of multiple subpasses
	// subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes (ex: post-processing a previous pass)
	// by grouping these rendering operations into a a single pass, Vulkan is able to reorder the operations and conserve memory bandwidth

	// a reference to the color attachment
	vk::AttachmentReference colorAttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

	// describe the subpass; where does it exist in the pipeline; what are its attachments
	vk::SubpassDescription subpass(
		{},
		vk::PipelineBindPoint::eGraphics,
		{},
		{},
		1,
		&colorAttachmentReference
	);

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
	vk::RenderPassCreateInfo renderPassInfo(
		{},
		1,
		&colorAttachment,
		1,
		&subpass
	);
	renderPass = device->createRenderPassUnique(renderPassInfo);

	std::cout << "[INFO] : " << "Created render pass" << std::endl;
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
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eClockwise,
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

	// depth and stencil testing goes here

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

	// create the graphics pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		{},
		0,
		nullptr,
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
		nullptr,					// pDepthStencilState
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
		std::array<vk::ImageView, 1> attachments = { view.get() };

		/*vk::ImageView attachments[] = {
			view.get()
		};*/

		//attachments[0] = view.get();

		swapChainFramebuffers.push_back(device->createFramebufferUnique(vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(),
			renderPass.get(),
			attachments,
			swapChainExtent.width,
			swapChainExtent.height,
			//surfaceData.extent.width,
			//surfaceData.extent.height,
			1
		)));
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
		vk::RenderPassBeginInfo renderPassInfo(renderPass.get(), swapChainFramebuffers[i].get());
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.renderArea.extent = swapChainExtent;

		vk::ClearValue clearValue(vk::ClearColorValue(std::array<uint32_t, 4>{0, 0, 0, 1}));

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		// begin the render pass
		commandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// bind the graphics pipeline
		commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());

		// bind vertex buffers
		std::array<vk::Buffer, 1> vertexBuffers = { vertexBufferData.buffer.get() };
		std::array<vk::DeviceSize, 1> offsets = { 0 };
		commandBuffers[i]->bindVertexBuffers(0, vertexBuffers, offsets);
		commandBuffers[i]->bindIndexBuffer(indexBufferData.buffer.get(), 0, vk::IndexType::eUint32);

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

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
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
