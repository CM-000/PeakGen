#pragma once
#include <glm/glm.hpp>

void setLightUniforms(unsigned int shaderProgram,
                      const glm::vec3& lightPos,
                      const glm::vec3& viewPos);