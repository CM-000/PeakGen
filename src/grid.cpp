#include "grid.h"

// This function builds a flat grid mesh made of triangles.
// Input: N = number of cells per side
// Output: vertices = list of 3D positions, indices = triangle connectivity
void generateGrid(int N, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    // Make sure the arrays are empty before we start
    vertices.clear();
    indices.clear();

    // Generate vertex positions
    // We want (N+1) points along each axis so we can form N cells.
    // Range is mapped from [0, N] → [-1, 1], so the grid spans -1 to +1.
    // Grid lies flat on the XZ plane (y = 0).
    for (int i = 0; i <= N; i++) {
        for (int j = 0; j <= N; j++) {
            float x = (float)i / (float)N * 2.0f - 1.0f;    // horizontal coordinate
            float z = (float)j / (float)N * 2.0f - 1.0f;    // depth coordinate
            float y = 0.0f;                                 // keep flat at ground level

            // Store the vertex (x, y, z)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // Generate triangle indices
    // Each cell (square) is made of 2 triangles.
    // Loop over all cells and connect the four corners.
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            // Calculate the indices of the four corners of the current cell
            int topLeft     = i * (N + 1) + j;
            int topRight    = topLeft + 1;
            int bottomLeft  = (i + 1) * (N + 1) + j;
            int bottomRight = bottomLeft + 1;

            // First triangle: topLeft → bottomLeft → topRight
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle: topRight → bottomLeft → bottomRight
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}