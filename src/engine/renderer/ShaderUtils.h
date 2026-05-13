#pragma once

#include <string>
#include <glad/glad.h>

namespace ShaderUtils {
    GLuint loadShaderProgram(const std::string& vertPath, const std::string& fragPath);
}
