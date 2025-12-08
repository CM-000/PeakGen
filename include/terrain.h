#pragma once
#include <vector>
#include <glm/glm.hpp>

// terrain generator
float generateTerrain(int N, std::vector<float>& vertices, std::vector<unsigned int>& indices);

// compute normals for the terrain mesh
void computeNormals(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, std::vector<float>& normals);