#pragma once
#include <vector>
#include "Vertex.h"
#include "BufferData.h"
#include "VulkanContext.h"
#include <iostream>

struct VulkanContext;
class Mesh;
class MeshFactory;

class Mesh
{
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	// Texture
	Mesh(MeshFactory& manager, VulkanContext* context, std::vector<Vertex> vertices, std::vector<uint32_t> indices) : vertices(vertices), indices(indices), manager(manager)
	{
		setupMesh(context);
		setrandoms();
	}

	void setrandoms()
	{
		float random = -1.f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1.f - (-1.f))));

		std::cout << "Random is " << random << std::endl;
	}

	vk::Buffer getVertexBufferData() { return vertexBufferData.buffer.get(); }
	vk::Buffer getIndexBufferData() { return indexBufferData.buffer.get(); }
	vk::DescriptorSet getDescriptorSet(int frame) { return descriptorSets[frame].get(); }
	//vk::Buffer getIndexBufferData() { return &indexBufferData; }

	void updateUniformBuffer(VulkanContext* context, uint32_t currentImage);

private:
	float xpos = 0.f;
	float random;

	MeshFactory& manager;

	BufferData vertexBufferData;
	BufferData indexBufferData;
	std::vector<BufferData> uniformBufferData;
	std::vector<BufferData> transformationBufferData;

	std::vector<vk::UniqueDescriptorSet> descriptorSets;

	void setupMesh(VulkanContext* context);
	void createVertexBuffer(VulkanContext* context);
	void createIndexBuffer(VulkanContext* context);
	void createUniformBuffer(VulkanContext* context);
	void createDescriptorSet(VulkanContext* context);
};


class MeshFactory
{
public:
	MeshFactory(VulkanContext* context) : context(context) { }

	Mesh& create(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
	{
		Mesh* mesh = new Mesh(*this, context, vertices, indices);
		std::unique_ptr<Mesh> uPtr{ mesh };
		meshes.emplace_back(std::move(uPtr));

		return *mesh;
	}

	// not great; correctly implemented, unique pointers should remove the need for this!
	void clean()
	{
		for (auto& mesh : meshes)
		{
			const auto meshPtr = mesh.release();
			delete meshPtr;
		}
	}

	//std::vector<std::unique_ptr<Mesh>>* getMeshes() { return &meshes; }

	Mesh* getMesh(const unsigned int iterator) { return meshes[iterator].get(); }
	int count() const { return meshes.size(); }

private:
	VulkanContext* context;

	std::vector<std::unique_ptr<Mesh>> meshes;
};