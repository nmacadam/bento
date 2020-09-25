#pragma once

#include <vulkan/vulkan.hpp>

#include "BufferData.h"
#include "ImageData.h"
#include "VulkanContext.h"
#include <glm/vec2.hpp>

namespace bento
{
	class ImGuiLayer
	{
	public:
		struct PushConstBlock {
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstBlock;

		// Options and values to display/toggle from the UI
		struct UISettings {
			bool displayModels = true;
			bool displayLogos = true;
			bool displayBackground = true;
			bool animateLight = false;
			float lightSpeed = 0.25f;
			std::array<float, 50> frameTimes{};
			float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
			float lightTimer = 0.0f;
		} uiSettings;

		ImGuiLayer(VulkanContext* vulkanContext);
		~ImGuiLayer();

		void initialize(vk::RenderPass renderPass, float width, float height);

		void newFrame(bool updateFrameGraph);
		void updateBuffers();
		void drawFrame(vk::CommandBuffer commandBuffer);

	private:
		VulkanContext* context;
		//vk::Device device;
		//vk::PhysicalDevice physicalDevice;

		vk::UniqueSampler sampler;
		BufferData vertexBufferData;
		BufferData indexBufferData;
		int32_t vertexCount = 0;
		int32_t indexCount = 0;

		ImageData fontImageData;
		vk::UniqueImageView fontImageView;

		vk::UniquePipelineCache pipelineCache;
		vk::UniquePipelineLayout pipelineLayout;
		vk::UniquePipeline pipeline;

		vk::UniqueDescriptorPool descriptorPool;
		vk::UniqueDescriptorSetLayout descriptorSetLayout;
		std::vector<vk::UniqueDescriptorSet> descriptorSets;

		void setImGuiStyle(float width, float height);
		void createResources(vk::RenderPass renderPass/*vk::RenderPass renderPass, vk::Queue copyQueue, const std::string& shadersPath*/);
	};
}
