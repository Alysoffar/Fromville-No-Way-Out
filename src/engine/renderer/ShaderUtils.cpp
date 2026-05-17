#include "engine/renderer/ShaderUtils.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace ShaderUtils {

static std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ShaderUtils] Could not open file: " << path << "\n";
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint loadShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSource = readFile(vertPath);
    std::string fragSource = readFile(fragPath);

    if (vertSource.empty() || fragSource.empty()) {
        std::cerr << "[ShaderUtils] Failed to read shader files: "
                  << vertPath << " / " << fragPath << "\n";
        return 0;
    }

    // Compile vertex shader
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vSrc = vertSource.c_str();
    glShaderSource(vertShader, 1, &vSrc, nullptr);
    glCompileShader(vertShader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetShaderInfoLog(vertShader, logLen, nullptr, log.data());
        std::cerr << "[ShaderUtils] Vertex shader compilation failed (" << vertPath << "):\n" << log << "\n";
        glDeleteShader(vertShader);
        return 0;
    }

    // Compile fragment shader
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fSrc = fragSource.c_str();
    glShaderSource(fragShader, 1, &fSrc, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetShaderInfoLog(fragShader, logLen, nullptr, log.data());
        std::cerr << "[ShaderUtils] Fragment shader compilation failed (" << fragPath << "):\n" << log << "\n";
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return 0;
    }

    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "[ShaderUtils] Shader program linking failed:\n" << log << "\n";
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(program);
        return 0;
    }

    // Clean up shader objects
    glDetachShader(program, vertShader);
    glDetachShader(program, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}

} // namespace ShaderUtils
