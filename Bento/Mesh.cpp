#include "Mesh.h"
#include <utility>
#include "VulkanUtils.h"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include "ObjectUBO.h"
#include <chrono>
//#include "VulkanUtils.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>




//Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
//{
//	this->vertices = std::move(vertices);
//	this->indices = std::move(indices);
//
//	setupMesh();
//}

//Mesh::Mesh(MeshFactory& manager, std::vector<Vertex> vertices, std::vector<uint32_t> indices)
//{
//	this->manager = manager;
//	this->vertices = std::move(vertices);
//	this->indices = std::move(indices);
//
//	setupMesh();
//}

void Mesh::updateUniformBuffer(VulkanContext* context, uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::rotate(model, time * glm::radians(90.0f), glm::vec3(0.5f, 0.0f, 1.0f));
	
	ObjectUBO ubo{};
	ubo.model = model;

	void* data = context->device.mapMemory(transformationBufferData[currentImage].memory.get(), 0, sizeof(ubo), {});
	memcpy(data, &ubo, sizeof(ubo));
	context->device.unmapMemory(transformationBufferData[currentImage].memory.get());
}

void Mesh::setupMesh(VulkanContext* context)
{
	createVertexBuffer(context);
	createIndexBuffer(context);
	createUniformBuffer(context);
	createDescriptorSet(context);
}

void Mesh::createVertexBuffer(VulkanContext* context)
{
	// create the staging buffer
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	BufferData stagingBufferData = VulkanUtils::createBuffer(
		context->device,
		bufferSize,
		context->physicalDevice,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);

	// copy the vertex data to the staging buffer
	uint8_t* data = static_cast<uint8_t*>(context->device.mapMemory(stagingBufferData.memory.get(), 0, bufferSize, {}));
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	context->device.unmapMemory(stagingBufferData.memory.get());

	// create the vertex buffer
	vertexBufferData = VulkanUtils::createBuffer(
		context->device,
		bufferSize,
		context->physicalDevice,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	// copy data from staging buffer to vertex buffer
	VulkanUtils::copyBuffer(
		context->device,
		context->commandPool,
		context->queue,
		stagingBufferData.buffer.get(),
		vertexBufferData.buffer.get(), bufferSize
	);
}

void Mesh::createIndexBuffer(VulkanContext* context)
{
	// create the staging buffer
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	BufferData stagingBufferData = VulkanUtils::createBuffer(
		context->device,
		bufferSize,
		context->physicalDevice,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);


	// copy the vertex data to the staging buffer
	uint8_t* data = static_cast<uint8_t*>(context->device.mapMemory(stagingBufferData.memory.get(), 0, bufferSize, {}));
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	context->device.unmapMemory(stagingBufferData.memory.get());

	// create the vertex buffer
	indexBufferData = VulkanUtils::createBuffer(
		context->device,
		bufferSize,
		context->physicalDevice,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	// copy data from staging buffer to vertex buffer
	VulkanUtils::copyBuffer(
		context->device,
		context->commandPool,
		context->queue,
		stagingBufferData.buffer.get(),
		indexBufferData.buffer.get(), bufferSize
	);
}

void Mesh::createUniformBuffer(VulkanContext* context)
{
	/*vk::DeviceSize bufferSize = sizeof(GlobalUBO);
	
	uniformBufferData.resize(context->swapChainImageCount);
	for (size_t i = 0; i < context->swapChainImageCount; i++) {
		uniformBufferData[i] = VulkanUtils::createBuffer(
			context->device,
			bufferSize,
			context->physicalDevice,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
	}*/

	vk::DeviceSize transformBufferSize = sizeof(ObjectUBO);
	transformationBufferData.resize(context->swapChainImageCount);

	for (size_t i = 0; i < context->swapChainImageCount; i++) {
		transformationBufferData[i] = VulkanUtils::createBuffer(
			context->device,
			transformBufferSize,
			context->physicalDevice,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
	}
}

void Mesh::createDescriptorSet(VulkanContext* context)
{
	std::vector<vk::DescriptorSetLayout> layouts(context->swapChainImageCount, context->descriptorSetLayout);

	vk::DescriptorSetAllocateInfo allocateInfo(context->descriptorPool, context->swapChainImageCount, layouts.data());

	descriptorSets.resize(context->swapChainImageCount);
	descriptorSets = context->device.allocateDescriptorSetsUnique(allocateInfo);

	// populate descriptors
	for (size_t i = 0; i < context->swapChainImageCount; i++) {
		//vk::DescriptorBufferInfo uniformBufferInfo(uniformBufferData[i].buffer.get(), 0, sizeof(GlobalUBO));
		vk::DescriptorBufferInfo modelMatInfo(transformationBufferData[i].buffer.get(), 0, sizeof(ObjectUBO));
		vk::DescriptorImageInfo imageInfo(context->sampler, context->imageView, vk::ImageLayout::eShaderReadOnlyOptimal);

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
			vk::WriteDescriptorSet(
				descriptorSets[i].get(),
				0,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&modelMatInfo,
				nullptr
			),
			vk::WriteDescriptorSet(
				descriptorSets[i].get(),
				1,
				0,
				1,
				vk::DescriptorType::eCombinedImageSampler,
				&imageInfo,
				nullptr,
				nullptr
			)
		};

		context->device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
}
