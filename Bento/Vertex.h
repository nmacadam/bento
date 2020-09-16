#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	// describes vertex data layout
	static vk::VertexInputBindingDescription getBindingDescription() {
		const vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
		return bindingDescription;
	}

	// describe individual data layout
	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
		const vk::VertexInputAttributeDescription position(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos));
		const vk::VertexInputAttributeDescription color(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));

		const std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = { position , color };

		return attributeDescriptions;
	}
};
