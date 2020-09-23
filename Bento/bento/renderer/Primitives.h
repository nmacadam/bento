#pragma once
#include <vector>

#include "Vertex.h"

class Plane
{
public:
	inline const static std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	inline const static  std::vector <uint32_t> indices = {
		0, 1, 2, 2, 3, 0
	};
};

class Quad
{
public:
	inline const static std::vector<Vertex> vertices = {
		{{-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	inline const static  std::vector <uint32_t> indices = {
		0, 1, 2, 2, 3, 0
	};
};

class Cube
{
public:
	inline const static std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.5f},	 {1.0f, 0.0f, 0.0f},	 {0.0f, 0.0f}},		// 0
		{{0.5f, -0.5f, 0.5f},	 {0.0f, 1.0f, 0.0f},	 {1.0f, 0.0f}},		// 1
		{{0.5f, 0.5f, 0.5f},		 {0.0f, 0.0f, 1.0f},	 {1.0f, 1.0f}},		// 2
		{{-0.5f, 0.5f, 0.5f},	 {1.0f, 1.0f, 1.0f},	 {0.0f, 1.0f}},		// 3

		{{-0.5f, -0.5f, -0.5f},	 {1.0f, 0.0f, 0.0f},	 {0.0f, 0.0f}},		// 4
		{{0.5f, -0.5f, -0.5f},	 {0.0f, 1.0f, 0.0f},	 {1.0f, 0.0f}},		// 5
		{{0.5f, 0.5f, -0.5f},	 {0.0f, 0.0f, 1.0f},	 {1.0f, 1.0f}},		// 6
		{{-0.5f, 0.5f, -0.5f},	 {1.0f, 1.0f, 1.0f},	 {0.0f, 1.0f}},		// 7
	};

	inline const static  std::vector <uint32_t> indices = {
		0, 1, 2, 2, 3, 0, // top
		1, 5, 6, 6, 2, 1, // front
		0, 4, 7, 7, 3, 0, // back
		0, 1, 5, 5, 4, 0, // right
		3, 2, 6, 6, 7, 3, // left
		4, 5, 6, 6, 7, 4  // bottom
	};
};