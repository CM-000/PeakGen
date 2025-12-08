#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "shader.h"
#include "camera.h"
#include "terrain.h"
#include "lighting.h"
#include "pathfinding.h"

// Window size
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Terrain shaders
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 viewPos;
uniform float maxHeight;
uniform vec3 lightDir;

void main() {
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
    vec3 specular = vec3(0.03) * spec;

    float h = clamp(FragPos.y / maxHeight, 0.0, 1.0);
    vec3 lowColor  = vec3(0.25, 0.25, 0.45);
    vec3 midColor  = vec3(0.35, 0.75, 0.35);
    vec3 highColor = vec3(0.85, 0.85, 0.85);

    vec3 baseColor = mix(lowColor, midColor, smoothstep(0.001, 0.05, h));
    baseColor = mix(baseColor, highColor, smoothstep(0.6, 1.0, h));

    vec3 rockColor = vec3(0.5, 0.5, 0.5);
    float slope = clamp(norm.y, 0.0, 1.0);
    float rockFactor = 1.0 - slope;
    baseColor = mix(baseColor, rockColor, 0.3 * rockFactor);

    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * baseColor;
    vec3 diffuse = 0.8 * diff * baseColor;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";

// Path shaders
const char* pathVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* pathFragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

const char* pointVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 model, view, projection;
void main() {
    vColor = aColor;
    gl_Position = projection * view * model * vec4(aPos,1.0);
}
)";

const char* pointFragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor,1.0); }
)";


// Helper to upload points (visited/frontier)
unsigned int uploadPoints(const std::vector<int>& indices, const std::vector<Node>& graph, const glm::vec3& color) {
    std::vector<float> data;
    for (int idx : indices) {
        const glm::vec3& p = graph[idx].position;
        data.push_back(p.x); data.push_back(p.y+0.02f); data.push_back(p.z);
        data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
    }
    unsigned int vao,vbo;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(float),data.data(),GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return vao;
}

int main() {
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PeakGen Terrain + Path", NULL, NULL);
    if (!window) {
        std::cerr << "Couldn't create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Couldn't initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h){ glViewport(0,0,w,h); });
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Compile shaders
    unsigned int terrainProgram = compileShader(vertexShaderSource, fragmentShaderSource);
    unsigned int pathProgram    = compileShader(pathVertexShader, pathFragmentShader);

    // Point shader for visited/frontier dots
    unsigned int pointProgram   = compileShader(pointVertexShader, pointFragmentShader);

    // Generate terrain ---------------------------------------------------
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    float maxHeight = generateTerrain(15, vertices, indices); // grid size

    std::vector<float> normals;
    computeNormals(vertices, indices, normals);

    // Interleave positions + normals
    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 2);
    for (size_t i = 0; i < vertices.size() / 3; i++) {
        vertexData.push_back(vertices[3*i]);
        vertexData.push_back(vertices[3*i+1]);
        vertexData.push_back(vertices[3*i+2]);
        vertexData.push_back(normals[3*i]);
        vertexData.push_back(normals[3*i+1]);
        vertexData.push_back(normals[3*i+2]);
    }

    // Upload terrain
    unsigned int terrainVAO, terrainVBO, terrainEBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glGenBuffers(1, &terrainEBO);

    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Convert vertices to glm::vec3
    std::vector<glm::vec3> positions;
    positions.reserve(vertices.size() / 3);
    for (size_t i = 0; i < vertices.size() / 3; ++i) {
        positions.emplace_back(vertices[3*i], vertices[3*i+1], vertices[3*i+2]);
    }

    // Pathfinding setup
    auto graph = buildGraph(positions, indices);

    // Strict tallest by y (object-space)
    int peakIndex = 0;
    float maxY = positions[0].y;

    for (int i = 1; i < (int)positions.size(); ++i) {
        if (positions[i].y > maxY) {
            maxY = positions[i].y;
            peakIndex = i;
        }
    }

    int startIndex = 0; // could be lowest corner
    Pathfinder pf(graph, startIndex, peakIndex);
    SearchState state;

    glLineWidth(3.0f);

    // Path line VAO/VBO (persistent)
    unsigned int pathVAO, pathVBO;
    glGenVertexArrays(1, &pathVAO);
    glGenBuffers(1, &pathVBO);

    glBindVertexArray(pathVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Visited points VAO/VBO (persistent)
    unsigned int visitedVAO, visitedVBO;
    glGenVertexArrays(1, &visitedVAO);
    glGenBuffers(1, &visitedVBO);

    glBindVertexArray(visitedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, visitedVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Frontier points VAO/VBO (persistent)
    unsigned int frontierVAO, frontierVBO;
    glGenVertexArrays(1, &frontierVAO);
    glGenBuffers(1, &frontierVBO);

    glBindVertexArray(frontierVAO);
    glBindBuffer(GL_ARRAY_BUFFER, frontierVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Render loop
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);

        // Draw terrain
        glUseProgram(terrainProgram);
        glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
        glUniform3fv(glGetUniformLocation(terrainProgram, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(terrainProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(terrainProgram, "maxHeight"), maxHeight);

        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Advance search one step and stop if path is found
        static double lastStepTime = 0.0;
        double now = glfwGetTime();

        // Only keep stepping if the path hasn't been found yet
        if (state.path.empty()) {
            if (now - lastStepTime > 0.1) {
                pf.step(state);
                lastStepTime = now;
            }
        }

        // Draw visited nodes (blue, smaller)
        if (!state.visited.empty()) {
            std::vector<float> data;
            for (int idx : state.visited) {
                const glm::vec3& p = graph[idx].position;
                data.insert(data.end(), {p.x, p.y+0.02f, p.z, 0.2f, 0.2f, 0.9f});
            }

            glPointSize(3.0f);
            glBindVertexArray(visitedVAO);
            glBindBuffer(GL_ARRAY_BUFFER, visitedVBO);
            glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_DYNAMIC_DRAW);

            glUseProgram(pointProgram);
            glUniformMatrix4fv(glGetUniformLocation(pointProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(pointProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(pointProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));

            glDrawArrays(GL_POINTS, 0, (GLsizei)state.visited.size());
            glBindVertexArray(0);
        }

        // Draw frontier nodes (orange, bigger)
        if (!state.frontier.empty()) {
            std::vector<float> data;
            for (int idx : state.frontier) {
                const glm::vec3& p = graph[idx].position;
                data.insert(data.end(), {p.x, p.y+0.02f, p.z, 0.9f, 0.5f, 0.1f});
            }

            glPointSize(10.0f);
            glBindVertexArray(frontierVAO);
            glBindBuffer(GL_ARRAY_BUFFER, frontierVBO);
            glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_DYNAMIC_DRAW);

            glUseProgram(pointProgram);
            glUniformMatrix4fv(glGetUniformLocation(pointProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(pointProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(pointProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));

            glDrawArrays(GL_POINTS, 0, (GLsizei)state.frontier.size());
            glBindVertexArray(0);
        }

        // Draw current best path (progressive line)
        int target = state.visited.empty() ? startIndex : state.visited.back();
        auto partialPath = pf.currentBestPath(target);
        if (!partialPath.empty()) {
            auto pathVertexData = buildPathVertexData(graph, partialPath);

            glBindVertexArray(pathVAO);
            glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
            glBufferData(GL_ARRAY_BUFFER,
                        pathVertexData.size() * sizeof(float),
                        pathVertexData.data(),
                        GL_DYNAMIC_DRAW);

            glUseProgram(pathProgram);
            glUniformMatrix4fv(glGetUniformLocation(pathProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(pathProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(pathProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));

            glDrawArrays(GL_LINES, 0, (GLsizei)pathVertexData.size()/6);
            glBindVertexArray(0);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);

    glDeleteVertexArrays(1, &pathVAO);
    glDeleteBuffers(1, &pathVBO);

    glDeleteVertexArrays(1, &visitedVAO);
    glDeleteBuffers(1, &visitedVBO);
    glDeleteVertexArrays(1, &frontierVAO);
    glDeleteBuffers(1, &frontierVBO);

    glfwTerminate();
    return 0;
}