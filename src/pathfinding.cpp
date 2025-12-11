#include "pathfinding.h"
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_set>
#include <glm/glm.hpp>

// Compute slope-based cost between two vertices
static float edgeCost(const glm::vec3& a, const glm::vec3& b) {
    float dy = std::abs(b.y - a.y);
    float dxz = glm::length(glm::vec2(b.x - a.x, b.z - a.z));
    float slope = dxz > 0.0f ? dy / dxz : 0.0f;
    return 1.0f + slope * 2.0f; // factor controls steepness penalty
}

std::vector<Node> buildGraph(const std::vector<glm::vec3>& vertices,
                             const std::vector<unsigned int>& indices) {
    std::vector<Node> graph(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        graph[i].position = vertices[i];
    }

    // Each triangle gives 3 edges
    for (size_t i = 0; i < indices.size(); i += 3) {
        int a = indices[i], b = indices[i+1], c = indices[i+2];
        float ab = edgeCost(vertices[a], vertices[b]);
        float bc = edgeCost(vertices[b], vertices[c]);
        float ca = edgeCost(vertices[c], vertices[a]);

        graph[a].neighbors.push_back({b, ab});
        graph[b].neighbors.push_back({a, ab});

        graph[b].neighbors.push_back({c, bc});
        graph[c].neighbors.push_back({b, bc});

        graph[c].neighbors.push_back({a, ca});
        graph[a].neighbors.push_back({c, ca});
    }
    return graph;
}

std::vector<int> findPath(const std::vector<Node>& graph,
                          int startIndex,
                          int goalIndex) {
    const float INF = std::numeric_limits<float>::infinity();
    std::vector<float> dist(graph.size(), INF);
    std::vector<int> prev(graph.size(), -1);

    struct Entry {
        int idx;
        float f;
        bool operator>(const Entry& other) const { return f > other.f; }
    };
    auto heuristic = [&](const glm::vec3& a, const glm::vec3& b) {
        return glm::length(a - b);
    };

    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> openSet;

    dist[startIndex] = 0.0f;
    float h0 = heuristic(graph[startIndex].position, graph[goalIndex].position);
    openSet.push({startIndex, h0});

    while (!openSet.empty()) {
        auto current = openSet.top(); openSet.pop();
        int u = current.idx;
        if (u == goalIndex) break;

        for (const Edge& e : graph[u].neighbors) {
            float tentative_g = dist[u] + e.cost;
            if (tentative_g < dist[e.to]) {
                dist[e.to] = tentative_g;
                prev[e.to] = u;
                float h = heuristic(graph[e.to].position, graph[goalIndex].position);
                float f = tentative_g + h * 15.0f; // more heuristic weight
                openSet.push({e.to, f});
            }
        }
    }

    std::vector<int> path;
    for (int v = goalIndex; v != -1; v = prev[v]) {
        path.push_back(v);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// Map slope to color: green (easy), yellow (moderate), red (hard)
static glm::vec3 slopeColor(float slope) {
    if (slope < 0.2f) return glm::vec3(0.1f, 0.9f, 0.1f);        // easy: green
    if (slope < 0.5f) return glm::vec3(0.95f, 0.8f, 0.2f);       // moderate: yellow
    return glm::vec3(0.9f, 0.15f, 0.15f);                        // hard: red
}

std::vector<float> buildPathVertexData(const std::vector<Node>& graph, const std::vector<int>& path) {
    std::vector<float> pathVertexData; // [x y z r g b] per vertex
    pathVertexData.reserve(path.size() * 12); // two vertices per segment

    auto computeSlope = [&](const glm::vec3& a, const glm::vec3& b) {
        float dy = std::abs(b.y - a.y);
        float dxz = glm::length(glm::vec2(b.x - a.x, b.z - a.z));
        return dxz > 0.0f ? dy / dxz : 0.0f;
    };

    for (size_t i = 0; i + 1 < path.size(); ++i) {
        const glm::vec3& a = graph[path[i]].position;
        const glm::vec3& b = graph[path[i+1]].position;
        float s = computeSlope(a, b);
        glm::vec3 c = slopeColor(s);

        // Vertex A
        pathVertexData.push_back(a.x); pathVertexData.push_back(a.y + 0.01f); pathVertexData.push_back(a.z);
        pathVertexData.push_back(c.r); pathVertexData.push_back(c.g); pathVertexData.push_back(c.b);
        // Vertex B
        pathVertexData.push_back(b.x); pathVertexData.push_back(b.y + 0.01f); pathVertexData.push_back(b.z);
        pathVertexData.push_back(c.r); pathVertexData.push_back(c.g); pathVertexData.push_back(c.b);
    }

    return pathVertexData;
}

// Pathfinder A* version (used to be djikstra)
Pathfinder::Pathfinder(const std::vector<Node>& g, int s, int goal)
    : graph(g), startIndex(s), goalIndex(goal),
      dist(g.size(), std::numeric_limits<float>::infinity()),
      prev(g.size(), -1)
{
    dist[startIndex] = 0.0f;
    float h = heuristic(graph[startIndex].position, graph[goalIndex].position);
    openSet.push({startIndex, h});
}

bool Pathfinder::step(SearchState& state) {
    if (openSet.empty()) return false;

    auto currentEntry = openSet.top();
    openSet.pop();
    int u = currentEntry.idx;

    // If already visited, skip
    if (visitedSet.count(u)) return true;
    visitedSet.insert(u);

    state.visited.push_back(u);

    if (u == goalIndex) {
        // reconstruct path
        state.path.clear();
        for (int v = goalIndex; v != -1; v = prev[v]) state.path.push_back(v);
        std::reverse(state.path.begin(), state.path.end());

        state.frontier.clear();
        return false; // search done
    }

    state.frontier.clear();
    for (const Edge& e : graph[u].neighbors) {
        float tentative_g = dist[u] + e.cost;
        if (tentative_g < dist[e.to]) {
            dist[e.to] = tentative_g;
            prev[e.to] = u;
            float h = heuristic(graph[e.to].position, graph[goalIndex].position);
            float f = tentative_g + h * 15.0f;
            openSet.push({e.to, f});
            state.frontier.push_back(e.to);
        }
    }
    return true; // search continues
}

std::vector<int> Pathfinder::currentBestPath(int target) const {
    std::vector<int> path;
    for (int v = target; v != -1; v = prev[v]) {
        path.push_back(v);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

float Pathfinder::heuristic(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b); // Euclidean distance
}