#include "terrain.h"
#include "PerlinNoise.hpp"
#include <vector>
#include <cmath>
#include <limits>
#include <glm/glm.hpp>
#include <iostream>
#include <random>

// Generate terrain with a mountain peak in the center
float generateTerrain(int N, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    // Random Perlin Noise
    // Good Map: 2022053872

    std::random_device rd;
    uint32_t seed = rd();
    siv::PerlinNoise perlin(seed); // replace with 'seed' for random

    std::cout<< "Perlin Seed: " << seed << std::endl;

    // Parameters to shape the mountain
    float scale     = 3.0f;   // zoom factor for noise
    float amplitude = 5.0f;   // maximum height
    float exponent  = 1.5f;   // sharpness of peak (use signed power)
    float radius    = 1.0f;   // falloff radius from center

    for (int i = 0; i <= N; i++) {
        for (int j = 0; j <= N; j++) {
            // Map grid coordinates to [-1, 1]
            float x = (float)i / (float)N * 2.0f - 1.0f;
            float z = (float)j / (float)N * 2.0f - 1.0f;

            // Base Perlin noise value in [0,1], remap to [-1,1]
            double n = perlin.noise2D(x * scale, z * scale);
            float noiseVal = static_cast<float>(n * 2.0 - 1.0);

            // Shrink valleys but keep sign
            if (noiseVal < 0.0f) noiseVal *= 0.2f;

            // Radial falloff so the highest point is near the center
            float dist = std::sqrt(x * x + z * z) / radius;
            float falloff = 1.0f - glm::clamp(dist, 0.0f, 1.0f);

            // Signed power to avoid NaNs when exponent is non-integer
            float h = noiseVal * falloff;
            float shaped = std::pow(std::abs(h), exponent);

            float y = amplitude * shaped;

            // Store vertex
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // compute min/max height
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (size_t i = 0; i < vertices.size(); i += 3) {
        float y = vertices[i+1];
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }

    std::cout << "Height range: " << minY << " to " << maxY << std::endl;

    // Build indices (CCW winding per quad)
    indices.reserve(N * N * 6);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int topLeft     = i * (N + 1) + j;
            int topRight    = topLeft + 1;
            int bottomLeft  = (i + 1) * (N + 1) + j;
            int bottomRight = bottomLeft + 1;

            // Triangle 1: topLeft -> bottomLeft -> bottomRight
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);

            // Triangle 2: topLeft -> bottomRight -> topRight
            indices.push_back(topLeft);
            indices.push_back(bottomRight);
            indices.push_back(topRight);
        }
    }
    return maxY;
}

// Compute vertex normals by averaging adjacent triangle normals
void computeNormals(const std::vector<float>& vertices,
                    const std::vector<unsigned int>& indices,
                    std::vector<float>& normals) {
    normals.assign(vertices.size(), 0.0f); // same size as vertices (x,y,z per vertex)

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 v0(vertices[3 * i0], vertices[3 * i0 + 1], vertices[3 * i0 + 2]);
        glm::vec3 v1(vertices[3 * i1], vertices[3 * i1 + 1], vertices[3 * i1 + 2]);
        glm::vec3 v2(vertices[3 * i2], vertices[3 * i2 + 1], vertices[3 * i2 + 2]);

        // Triangle normal (use consistent order with CCW indices)
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 triNormal = glm::cross(edge1, edge2);

        // Guard against degenerate triangles
        float len2 = glm::dot(triNormal, triNormal);
        if (len2 > 1e-12f) {
            triNormal *= 1.0f / std::sqrt(len2);

            normals[3 * i0]     += triNormal.x; normals[3 * i0 + 1] += triNormal.y; normals[3 * i0 + 2] += triNormal.z;
            normals[3 * i1]     += triNormal.x; normals[3 * i1 + 1] += triNormal.y; normals[3 * i1 + 2] += triNormal.z;
            normals[3 * i2]     += triNormal.x; normals[3 * i2 + 1] += triNormal.y; normals[3 * i2 + 2] += triNormal.z;
        }
    }

    // Normalize all vertex normals
    for (size_t i = 0; i < vertices.size(); i += 3) {
        glm::vec3 n(normals[i], normals[i + 1], normals[i + 2]);
        float len2 = glm::dot(n, n);
        if (len2 > 1e-12f) {
            n *= 1.0f / std::sqrt(len2);
        } else {
            n = glm::vec3(0.0f, 1.0f, 0.0f); // fallback up
        }
        normals[i] = n.x; normals[i + 1] = n.y; normals[i + 2] = n.z;
    }
}