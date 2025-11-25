#include "grid.h"

// Create (N+1) x (N+1) vertices so we have N cells per side.
// Each cell is two triangles. Indices form GL_TRIANGLES.
void generateGrid(int N, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    // Generate positions in [-1, 1] for X and Y, Z = 0
    for (int i = 0; i <= N; i++) {
        for (int j = 0; j <= N; j++) {
            float x = (float)i / (float)N * 2.0f - 1.0f; // left-right
            float z = (float)j / (float)N * 2.0f - 1.0f; // forward-back
            float y = 0.0f;                              // flat on ground

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // Build two triangles per cell
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int topLeft     = i * (N + 1) + j;
            int topRight    = topLeft + 1;
            int bottomLeft  = (i + 1) * (N + 1) + j;
            int bottomRight = bottomLeft + 1;

            // Triangle 1: topLeft, bottomLeft, topRight
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Triangle 2: topRight, bottomLeft, bottomRight
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}
