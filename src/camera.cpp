#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Camera globals
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.5f, 3.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, -0.3f, -1.0f));
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
float fov = 45.0f;

static float yaw = -90.0f;
static float pitch = 0.0f;
static float lastX = 400.0f, lastY = 300.0f; // explicitly floats
static bool firstMouse = true;

void processInput(GLFWwindow* window, float deltaTime) {
    float speed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront; // forward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront; // back
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)                                   // left
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;         
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)                                   // right
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;         

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos -= speed * cameraUp;    // down
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += speed * cameraUp;    // up

    // Wireframe toggle: press F once to switch, again to revert
    static bool fWasDown = false;
    static bool wireframe = false;
    bool fIsDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (fIsDown && !fWasDown) {
        wireframe = !wireframe;
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    fWasDown = fIsDown;

    // Debug hotkey: print once when P is pressed
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = static_cast<float>(cos(glm::radians(yaw)) * cos(glm::radians(pitch)));
    front.y = static_cast<float>(sin(glm::radians(pitch)));
    front.z = static_cast<float>(sin(glm::radians(yaw)) * cos(glm::radians(pitch)));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= static_cast<float>(yoffset);
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}