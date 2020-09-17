#pragma once
#include <glm/mat4x4.hpp>

// Note: Make sure to consider alignment in memory...
// -Scalars have to be aligned by N(= 4 bytes given 32 bit floats).
// -A vec2 must be aligned by 2N(= 8 bytes)
// -A vec3 or vec4 must be aligned by 4N(= 16 bytes)
// -A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
// -A mat4 matrix must have the same alignment as a vec4.
// full list: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap14.html#interfaces-resources-layout

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
