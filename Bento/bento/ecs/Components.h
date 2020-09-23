#pragma once

#include <glm/glm.hpp>

namespace bento
{
	struct TagComponent
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: tag(tag) {}
	};

	struct TransformComponent
	{
		glm::mat4 transform{1.0f};

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::mat4& transform)
			: transform(transform) {}
	};

	struct MeshComponent
	{
		glm::vec4 color{1.0f};

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(const glm::vec4& color)
			: color(color) {}
	};
}