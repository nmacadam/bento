#include "bpch.h"
#include "ImGuiLayer.h"
#include "VulkanUtils.h"

#include <imgui.h>
#include "bento/core/log.h"
#include "Shader.h"

namespace bento
{
	ImGuiLayer::ImGuiLayer(VulkanContext* vulkanContext)
	{
		bento::log::warn("doing imgui things");
		this->context = vulkanContext;

		ImGui::CreateContext();
	}

	ImGuiLayer::~ImGuiLayer()
	{
		ImGui::DestroyContext();
	}

	void ImGuiLayer::initialize(vk::RenderPass renderPass, float width, float height)
	{
		log::info("Initializing ImGui layer...");

		vertexBufferData.device = context->device;
		indexBufferData.device = context->device;

		setImGuiStyle(width, height);
		createResources(renderPass);

		log::info("ImGui layer initialized");
	}

	void ImGuiLayer::drawFrame(vk::CommandBuffer commandBuffer)
	{
		ImGuiIO& io = ImGui::GetIO();

		// worried about this one
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout.get(),
			0,
			1, &descriptorSets[0].get(),
			0, nullptr
		);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

		vk::Viewport viewport(0.0f, 0.0f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
		commandBuffer.setViewport(0, 1, &viewport);

		// UI scale and translate via push constants
		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = glm::vec2(-1.0f);
		commandBuffer.pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstBlock), &pushConstBlock);

		// Render commands
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if (imDrawData->CmdListsCount > 0) {

			vk::DeviceSize offsets[1] = { 0 };
			commandBuffer.bindVertexBuffers(0, 1, &vertexBufferData.buffer.get(), offsets);
			commandBuffer.bindIndexBuffer(indexBufferData.buffer.get(), 0, vk::IndexType::eUint16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					vk::Rect2D scissorRect;
					scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					commandBuffer.setScissor(0, 1, &scissorRect);
					commandBuffer.drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}
	}

	void ImGuiLayer::setImGuiStyle(float width, float height)
	{
		// Color scheme
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

		// Dimensions
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(width, height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	}

	void ImGuiLayer::createResources(vk::RenderPass renderPass/*vk::RenderPass renderPass, vk::Queue copyQueue, const std::string& shadersPath*/)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		unsigned char* fontData;
		int texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

		// Create target image for copy
		fontImageData = VulkanUtils::createImage(
			context->device,
			context->physicalDevice,
			texWidth,
			texHeight,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);
		log::trace("Created font image");

		// prepare the image to be copied (using transfer destination optimal)
		VulkanUtils::transitionImageLayout(
			context->device, context->commandPool, context->queue,
			fontImageData.image.get(),
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal
		);

		// Image view
		fontImageView = VulkanUtils::createImageView(
			context->device,
			fontImageData.image.get(),
			//vk::Format::eR8G8B8A8Srgb,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageAspectFlagBits::eColor
		);
		log::trace("Created font image view");

		// Staging buffers for font data upload
		vk::DeviceSize bufferSize = texWidth * texHeight * 4 * sizeof(char);
		BufferData stagingBufferData = VulkanUtils::createBuffer(
			context->device,
			bufferSize,
			context->physicalDevice,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
		log::trace("   Created staging buffer");

		// copy the data to the staging buffer
		void* data = context->device.mapMemory(stagingBufferData.memory.get(), 0, bufferSize, {});
		memcpy(data, fontData, static_cast<size_t>(bufferSize));
		context->device.unmapMemory(stagingBufferData.memory.get());
		log::trace("   Copied data to staging buffer");

		// copy staging buffer contents to image
		VulkanUtils::copyBufferToImage(
			context->device, context->commandPool, context->queue,
			stagingBufferData.buffer.get(),
			fontImageData.image.get(),
			texWidth,
			texHeight
		);
		log::trace("   Copied staging buffer contents to image");
		log::trace("Copied font data to image");

		// Prepare for shader read
		VulkanUtils::transitionImageLayout(
			context->device, context->commandPool, context->queue,
			fontImageData.image.get(),
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);
		log::warn("transitioned image layout for shader access");

		/*vks::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);*/

		// mess with these
		vk::SamplerCreateInfo samplerInfo(
			{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge,
			0.0f,
			true, 16.0f,
			false, vk::CompareOp::eAlways,
			0.0f, 0.0f,
			vk::BorderColor::eFloatOpaqueWhite,
			false
		);
		sampler = context->device.createSamplerUnique(samplerInfo);
		log::trace("Created sampler");

		// set up descriptor
		// Descriptor pool
		std::array<vk::DescriptorPoolSize, 1> poolSizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
		};

		// check this
		vk::DescriptorPoolCreateInfo poolInfo({}, 2, 1, poolSizes.data());
		descriptorPool = context->device.createDescriptorPoolUnique(poolInfo);
		log::trace("Created descriptor pool");

		// Descriptor set layout
		std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings = {
			vk::DescriptorSetLayoutBinding(
				0, 
				vk::DescriptorType::eCombinedImageSampler, 
				1,
				vk::ShaderStageFlagBits::eFragment
			)
		};
		vk::DescriptorSetLayoutCreateInfo descriptorLayoutInfo({}, setLayoutBindings.size(), setLayoutBindings.data());
		descriptorSetLayout = context->device.createDescriptorSetLayoutUnique(descriptorLayoutInfo);
		log::trace("Created descriptor set layout");

		// Descriptor set
		vk::DescriptorSetAllocateInfo allocateInfo(descriptorPool.get(), 1, &descriptorSetLayout.get());
		descriptorSets.resize(1);
		descriptorSets = context->device.allocateDescriptorSetsUnique(allocateInfo);
		log::trace("Allocated descriptor set");

		vk::DescriptorImageInfo fontDescriptor(sampler.get(), fontImageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal);

		std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
			vk::WriteDescriptorSet(
				descriptorSets[0].get(),
				0,
				0,
				1,
				vk::DescriptorType::eCombinedImageSampler,
				&fontDescriptor,
				nullptr,
				nullptr
			)
		};

		context->device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		log::trace("Updated descriptor set");

		// Pipeline cache
		pipelineCache = context->device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
		log::trace("Created pipeline cache");

		// Pipeline layout --------------------------

		// Push constants for UI rendering parameters
		vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstBlock));

		// create pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, 1, &descriptorSetLayout.get(), 1, &pushConstantRange);
		pipelineLayout = context->device.createPipelineLayoutUnique(pipelineLayoutCreateInfo);
		log::trace("Created pipeline layout");

		// Setup graphics pipeline for UI rendering
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState({}, vk::PrimitiveTopology::eTriangleList, false);

		vk::PipelineRasterizationStateCreateInfo rasterizationState(
			{},
			false,
			false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			{}, {}, {}, {},
			1.0f
		);

		// Enable blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			true,
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		);

		vk::PipelineColorBlendStateCreateInfo colorBlendState({}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

		vk::PipelineDepthStencilStateCreateInfo depthStencil({},false,false,vk::CompareOp::eLessOrEqual);

		vk::Viewport viewport(
			0.0f,
			0.0f,
			static_cast<float>(context->swapChainExtent.width),
			static_cast<float>(context->swapChainExtent.height),
			0.0f,
			1.0f
		);

		vk::Rect2D scissor(
			{ 0, 0 },
			context->swapChainExtent
		);

		vk::PipelineViewportStateCreateInfo viewportState(
			{},
			1,
			&viewport,
			1,
			&scissor
		);

		// check
		vk::PipelineMultisampleStateCreateInfo multisampleState(
			{},
			vk::SampleCountFlagBits::e1,
			false,
			1.0f,
			nullptr,
			false,
			false
		);
		log::warn("Disabling multisampling");

		std::vector<vk::DynamicState> dynamicStateEnables = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

		vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStateEnables.size(), dynamicStateEnables.data());

		// Vertex bindings an attributes based on ImGui vertex definition
		std::vector<vk::VertexInputBindingDescription> vertexInputBindings = {
			vk::VertexInputBindingDescription(0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex)
		};
		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes = {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)),	// Location 0: Position
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)),	// Location 1: UV
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)), // Location 2: Color
		};
		vk::PipelineVertexInputStateCreateInfo vertexInputState(
			{},
			vertexInputBindings.size(),
			vertexInputBindings.data(),
			vertexInputAttributes.size(),
			vertexInputAttributes.data()
		);

		// do something about the paths
		Shader vertexShader(context->device, "../shaders/ui_vert.spv");
		Shader fragmentShader(context->device, "../shaders/ui_frag.spv");

		//Shader vertexShader(context->device, "shaders/vert.spv");
		//Shader fragmentShader(context->device, "shaders/frag.spv");

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

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
			{},									// flags
			shaderStages,								// stages
			&vertexInputState,							// pVertexInputState
			&inputAssemblyState,						// pInputAssemblyState
			nullptr,					// pTessellationState
			&viewportState,								// pViewportState
			&rasterizationState,						// pRasterizationState
			&multisampleState,							// pMultisampleState
			&depthStencil,								// pDepthStencilState
			&colorBlendState,							// pColorBlendState
			nullptr,						// pDynamicState
			pipelineLayout.get(),					// layout
			renderPass									// renderPass
		);
		pipeline = context->device.createGraphicsPipelineUnique(pipelineCache.get(), graphicsPipelineCreateInfo);

		log::trace("Created pipeline");
	}

	void ImGuiLayer::newFrame(bool updateFrameGraph)
	{
		ImGui::NewFrame();

		// Init imGui windows and elements

		//ImVec4 clear_color = ImColor(114, 144, 154);
		//static float f = 0.0f;
		//ImGui::TextUnformatted("bento");
		//ImGui::TextUnformatted(VulkanUtils::getDeviceName(context->physicalDevice));

		//ImGui::End();

		//// Update frame time display
		//if (updateFrameGraph) {
		//	std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
		//	//float frameTime = 1000.0f / (example->frameTimer * 1000.0f);
		//	float frameTime = 10f;
		//	uiSettings.frameTimes.back() = frameTime;
		//	if (frameTime < uiSettings.frameTimeMin) {
		//		uiSettings.frameTimeMin = frameTime;
		//	}
		//	if (frameTime > uiSettings.frameTimeMax) {
		//		uiSettings.frameTimeMax = frameTime;
		//	}
		//}

		//ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));

		//ImGui::Text("Camera");
		////ImGui::InputFloat3("position", &example->camera.position.x, 2);
		////ImGui::InputFloat3("rotation", &example->camera.rotation.x, 2);

		////ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
		////ImGui::Begin("Example settings");
		////ImGui::Checkbox("Render models", &uiSettings.displayModels);
		////ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
		////ImGui::Checkbox("Display background", &uiSettings.displayBackground);
		////ImGui::Checkbox("Animate light", &uiSettings.animateLight);
		////ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
		////ImGui::End();

		//ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowDemoWindow();

		// Render to generate draw buffers
		ImGui::Render();
	}

	void ImGuiLayer::updateBuffers()
	{
		ImDrawData* imDrawData = ImGui::GetDrawData();

		vk::DeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		vk::DeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		// Update buffers only if vertex or index count has been changed compared to current buffer size

		// Vertex buffer
		//if ((vertexBufferData.buffer.get() == nullptr) || (vertexCount != imDrawData->TotalVtxCount)) {
		//	context->device.unmapMemory(vertexBufferData.memory.get());
		//	context->device.destroyBuffer(vertexBufferData.buffer.release());
		//	context->device.freeMemory(vertexBufferData.memory.release());

		//	vertexBufferData = VulkanUtils::createBuffer(
		//		context->device,
		//		vertexBufferSize,
		//		context->physicalDevice,
		//		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		//		vk::MemoryPropertyFlagBits::eHostVisible
		//	);

		//	vertexCount = imDrawData->TotalVtxCount;

		//	// map?
		//	void* mapped = nullptr;
		//	context->device.mapMemory(vertexBufferData.memory.get(), 0, VK_WHOLE_SIZE, {}, &mapped);
		//}

		//if ((indexBufferData.buffer.get() == nullptr) || (indexCount != imDrawData->TotalIdxCount)) {
		//	context->device.unmapMemory(indexBufferData.memory.get());
		//	context->device.destroyBuffer(indexBufferData.buffer.release());
		//	context->device.freeMemory(indexBufferData.memory.release());

		//	indexBufferData = VulkanUtils::createBuffer(
		//		context->device,
		//		indexBufferSize,
		//		context->physicalDevice,
		//		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		//		vk::MemoryPropertyFlagBits::eHostVisible
		//	);

		//	indexCount = imDrawData->TotalIdxCount;

		//	// map?
		//	void* mapped = nullptr;
		//	context->device.mapMemory(indexBufferData.memory.get(), 0, VK_WHOLE_SIZE, {}, &mapped);
		//}

		// Vertex buffer
		if ((vertexBufferData.buffer.get() == nullptr) || (vertexCount != imDrawData->TotalVtxCount)) {
			vertexBufferData.unmap();
			vertexBufferData.destroy();
			
			vertexBufferData = VulkanUtils::createBuffer(
				context->device,
				vertexBufferSize,
				context->physicalDevice,
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible
			);

			vertexCount = imDrawData->TotalVtxCount;
			vertexBufferData.map();
		}

		// Index buffer
		if ((indexBufferData.buffer.get() == nullptr) || (indexCount < imDrawData->TotalIdxCount)) {
			indexBufferData.unmap();
			indexBufferData.destroy();
			
			indexBufferData = VulkanUtils::createBuffer(
				context->device,
				indexBufferSize,
				context->physicalDevice,
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible
			);

			indexCount = imDrawData->TotalIdxCount;
			indexBufferData.map();
		}

		// Upload data
		ImDrawVert* vtxDst = (ImDrawVert*)vertexBufferData.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)indexBufferData.mapped;

		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// Flush to make writes visible to GPU
		vertexBufferData.flush();
		indexBufferData.flush();
	}
}
