#include "lighting.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

void setLightUniforms(unsigned int shaderProgram,
                      const glm::vec3& lightPos,
                      const glm::vec3& viewPos) {
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
}