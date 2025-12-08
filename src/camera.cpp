#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Camera settings
// Position: where the camera is in the world
// Front: the direction the camera is looking
// Up: which way is "up" for the camera
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.5f, 3.0f);                    // start slightly above ground, a bit back
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, -0.3f, -1.0f));  // looking forward and slightly down
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);                    // world up is positive Y
float fov = 45.0f;                                                      // field of view (zoom level)

// Angles used to calculate where the camera is pointing
static float yaw = -90.0f;                      // horizontal angle (start facing -Z)
static float pitch = 0.0f;                      // vertical angle (start level)
static float lastX = 400.0f, lastY = 300.0f;    // last mouse position (center of screen)
static bool firstMouse = true;                  // flag to avoid jump on first mouse move

// Handle keyboard input
void processInput(GLFWwindow* window, float deltaTime) {
    float speed = 2.5f * deltaTime; // movement speed depends on frame time

    // Move forward/backward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;

    // Move left/right (strafe)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

    // Move up/down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos -= speed * cameraUp;    // up
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += speed * cameraUp;    // down

    // Toggle wireframe mode with F
    static bool fWasDown = false;
    static bool wireframe = false;
    bool fIsDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (fIsDown && !fWasDown) {
        wireframe = !wireframe;
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // show mesh as lines
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // show solid triangles
    }
    fWasDown = fIsDown;

    // Print camera info with P
    static bool pWasDown = false;
    bool pIsDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pIsDown && !pWasDown) {
        std::cout << "\n----------------------------------------\n";
        std::cout << "cameraPos:   " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << "\n";
        std::cout << "cameraFront: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << "\n";
        std::cout << "cameraUp:    " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << "\n";
    }
    pWasDown = pIsDown;
}

// Handle mouse movement (rotate camera)
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // First mouse event: just set lastX/lastY so we don’t get a huge jump
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    // Calculate how far the mouse moved
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // reversed so moving mouse up increases pitch
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    // Apply sensitivity
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Update yaw and pitch
    yaw += xoffset;
    pitch += yoffset;

    // Limit pitch so the camera doesn’t flip upside down
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Recalculate the camera front vector from yaw/pitch
    glm::vec3 front;
    front.x = static_cast<float>(cos(glm::radians(yaw)) * cos(glm::radians(pitch)));
    front.y = static_cast<float>(sin(glm::radians(pitch)));
    front.z = static_cast<float>(sin(glm::radians(yaw)) * cos(glm::radians(pitch)));
    cameraFront = glm::normalize(front);
}

// Handle scroll wheel (zoom in/out)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= static_cast<float>(yoffset);     // zoom changes field of view
    if (fov < 1.0f) fov = 1.0f;             // don't zoom too far in
    if (fov > 45.0f) fov = 45.0f;           // don't zoom too far out
}