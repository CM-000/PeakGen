#pragma once
#include <vector>
#include <queue>
#include <functional>
#include <glm/glm.hpp>
#include <cmath>
#include <unordered_set>

// Edge in the graph
struct Edge {
    int to;
    float cost;
};

// Graph node
struct Node {
    glm::vec3 position;
    std::vector<Edge> neighbors;
};

// Build adjacency list from terrain vertices/indices
std::vector<Node> buildGraph(const std::vector<glm::vec3>& vertices,
                             const std::vector<unsigned int>& indices);

// Convert path indices into renderable vertex data [x y z r g b]
std::vector<float> buildPathVertexData(const std::vector<Node>& graph,
                                       const std::vector<int>& path);

// Live search visualization
struct SearchState {
    std::vector<int> visited;   // nodes expanded so far
    std::vector<int> frontier;  // nodes discovered this step
    std::vector<int> path;      // final path once goal reached
};

class Pathfinder {
public:
    Pathfinder(const std::vector<Node>& g, int s, int goal);
    bool step(SearchState& state); // advance one iteration, fill state

    std::vector<int> currentBestPath(int target) const;

private:
    const std::vector<Node>& graph;
    int startIndex, goalIndex;
    std::vector<float> dist;
    std::vector<int> prev;

    struct NodeEntry {
        int idx;
        float f;
        bool operator>(const NodeEntry& other) const { return f > other.f; }
    };

    std::priority_queue<NodeEntry, std::vector<NodeEntry>, std::greater<NodeEntry>> openSet;
    std::unordered_set<int> visitedSet;

    static float heuristic(const glm::vec3& a, const glm::vec3& b);
};