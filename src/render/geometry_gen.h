#pragma once

#include "vector"

#include "glm/glm.hpp"

#include "render/vertex.h"

namespace geometry_gen
{
	std::vector<Vertex> generateGrid(int half = 10, float step = 1.0f);
	std::vector<Vertex> generateCubeWire(float s = 0.5f);
	std::vector<glm::vec3> generateCubeSolidPositions(float s = 0.5f);
}