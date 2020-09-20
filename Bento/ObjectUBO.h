#pragma once
#include <glm/mat4x4.hpp>

struct ObjectUBO {
	alignas(16) glm::mat4 model;
};
