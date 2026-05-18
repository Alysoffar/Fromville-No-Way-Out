#include "engine/renderer/Shader.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef GL_NUM_PROGRAM_BINARY_FORMATS
#define GL_NUM_PROGRAM_BINARY_FORMATS 0x87FE
#endif

#ifndef GL_PROGRAM_BINARY_LENGTH
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#endif

typedef void (APIENTRY * PFNGLPROGRAMBINARYPROC) (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
typedef void (APIENTRY * PFNGLGETPROGRAMBINARYPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);

static PFNGLPROGRAMBINARYPROC My_glProgramBinary = nullptr;
static PFNGLGETPROGRAMBINARYPROC My_glGetProgramBinary = nullptr;

static void ResolveProgramBinaryFunctions() {
    static bool resolved = false;
    if (resolved) return;
    resolved = true;
#ifdef _WIN32
    My_glProgramBinary = (PFNGLPROGRAMBINARYPROC)wglGetProcAddress("glProgramBinary");
    My_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)wglGetProcAddress("glGetProgramBinary");
#endif
}

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

#include <glm/gtc/type_ptr.hpp>
#include "engine/core/StartupTimer.h"

Shader::Shader(const std::string& shaderName)
	: programID(0), name(shaderName) {}

std::string Shader::LoadFile(const std::string& path) {
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path);
	}
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

GLuint Shader::CompileShader(GLenum type, const std::string& source) {
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE) {
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(static_cast<std::size_t>(logLength), '\0');
		glGetShaderInfoLog(shader, logLength, nullptr, log.data());
		glDeleteShader(shader);
		throw std::runtime_error(log);
	}

	return shader;
}

void Shader::Load(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath) {
	StartupTimer::Begin("Shader Load: " + name);
	if (programID != 0) {
		glDeleteProgram(programID);
		programID = 0;
	}

	ResolveProgramBinaryFunctions();

	GLint numFormats = 0;
	if (My_glProgramBinary && My_glGetProgramBinary) {
		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormats);
	}
	
	std::string binPath = "shaders/cache/" + name + ".bin";
	std::string fmtPath = "shaders/cache/" + name + ".fmt";
	bool useCache = false;

	if (numFormats > 0 && std::filesystem::exists(binPath) && std::filesystem::exists(fmtPath)) {
		try {
			auto binTime = std::filesystem::last_write_time(binPath);
			auto vertTime = std::filesystem::last_write_time(vertPath);
			auto fragTime = std::filesystem::last_write_time(fragPath);
			bool geomValid = true;
			if (!geomPath.empty()) {
				auto geomTime = std::filesystem::last_write_time(geomPath);
				if (geomTime > binTime) {
					geomValid = false;
				}
			}
			if (vertTime <= binTime && fragTime <= binTime && geomValid) {
				useCache = true;
			}
		} catch (const std::exception& e) {
			std::cout << "[ShaderCache] Failed to validate write times: " << e.what() << "\n";
		}
	}

	if (useCache) {
		try {
			std::ifstream fmtFile(fmtPath, std::ios::in);
			GLenum format = 0;
			if (fmtFile >> format) {
				std::ifstream binFile(binPath, std::ios::in | std::ios::binary);
				binFile.seekg(0, std::ios::end);
				std::streamsize size = binFile.tellg();
				binFile.seekg(0, std::ios::beg);

				std::vector<char> buffer(static_cast<std::size_t>(size));
				if (binFile.read(buffer.data(), size)) {
					programID = glCreateProgram();
					My_glProgramBinary(programID, format, buffer.data(), static_cast<GLsizei>(size));

					GLint linked = GL_FALSE;
					glGetProgramiv(programID, GL_LINK_STATUS, &linked);
					if (linked == GL_TRUE) {
						std::cout << "[ShaderCache] Loaded shader binary from cache: " << name << "\n";
						uniformCache.clear();
						StartupTimer::End("Shader Load: " + name);
						return;
					} else {
						std::cout << "[ShaderCache] Link failed for binary: " << name << ", recompiling from source...\n";
						glDeleteProgram(programID);
						programID = 0;
					}
				}
			}
		} catch (const std::exception& e) {
			std::cout << "[ShaderCache] Exception loading binary cache: " << e.what() << ", recompiling...\n";
			if (programID != 0) {
				glDeleteProgram(programID);
				programID = 0;
			}
		}
	}

	const std::string vertexSource = LoadFile(vertPath);
	const std::string fragmentSource = LoadFile(fragPath);

	GLuint vertexShader = 0;
	GLuint fragmentShader = 0;
	GLuint geometryShader = 0;

	try {
		vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
	} catch (const std::exception& error) {
		throw std::runtime_error("Shader '" + name + "' vertex compilation failed for " + vertPath + ": " + error.what());
	}

	try {
		fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	} catch (const std::exception& error) {
		glDeleteShader(vertexShader);
		throw std::runtime_error("Shader '" + name + "' fragment compilation failed for " + fragPath + ": " + error.what());
	}

	if (!geomPath.empty()) {
		const std::string geometrySource = LoadFile(geomPath);
		try {
			geometryShader = CompileShader(GL_GEOMETRY_SHADER, geometrySource);
		} catch (const std::exception& error) {
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			throw std::runtime_error("Shader '" + name + "' geometry compilation failed for " + geomPath + ": " + error.what());
		}
	}

	programID = glCreateProgram();
	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);
	if (geometryShader != 0) {
		glAttachShader(programID, geometryShader);
	}
	glLinkProgram(programID);

	GLint linked = GL_FALSE;
	glGetProgramiv(programID, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE) {
		GLint logLength = 0;
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(static_cast<std::size_t>(logLength), '\0');
		glGetProgramInfoLog(programID, logLength, nullptr, log.data());
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		if (geometryShader != 0) {
			glDeleteShader(geometryShader);
		}
		glDeleteProgram(programID);
		programID = 0;
		throw std::runtime_error("Shader '" + name + "' link failed: " + log);
	}

	glDetachShader(programID, vertexShader);
	glDetachShader(programID, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	if (geometryShader != 0) {
		glDetachShader(programID, geometryShader);
		glDeleteShader(geometryShader);
	}

	if (numFormats > 0 && programID != 0) {
		GLint binaryLength = 0;
		glGetProgramiv(programID, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
		if (binaryLength > 0) {
			std::vector<char> binaryBuffer(static_cast<std::size_t>(binaryLength));
			GLenum format = 0;
			GLsizei length = 0;
			My_glGetProgramBinary(programID, binaryLength, &length, &format, binaryBuffer.data());
			if (length > 0) {
				try {
					std::filesystem::create_directories("shaders/cache");
					std::ofstream binFile(binPath, std::ios::out | std::ios::binary);
					binFile.write(binaryBuffer.data(), length);
					binFile.close();

					std::ofstream fmtFile(fmtPath, std::ios::out);
					fmtFile << format;
					fmtFile.close();
					std::cout << "[ShaderCache] Program binary successfully cached for: " << name << "\n";
				} catch (const std::exception& e) {
					std::cout << "[ShaderCache] Failed to write cache: " << e.what() << "\n";
				}
			}
		}
	}

	uniformCache.clear();
	StartupTimer::End("Shader Load: " + name);
}

void Shader::Bind() const {
	glUseProgram(programID);
}

void Shader::Unbind() const {
	glUseProgram(0);
}

GLuint Shader::GetID() const {
	return programID;
}

GLint Shader::GetUniformLocation(const std::string& uniformName) {
	auto found = uniformCache.find(uniformName);
	if (found != uniformCache.end()) {
		return found->second;
	}

	const GLint location = glGetUniformLocation(programID, uniformName.c_str());
	if (location == -1) {
		std::cerr << "Shader '" << name << "' warning: uniform not found: " << uniformName << '\n';
	}
	uniformCache.emplace(uniformName, location);
	return location;
}

void Shader::SetBool(const std::string& uniformName, bool value) {
	glUniform1i(GetUniformLocation(uniformName), value ? 1 : 0);
}

void Shader::SetInt(const std::string& uniformName, int value) {
	glUniform1i(GetUniformLocation(uniformName), value);
}

void Shader::SetFloat(const std::string& uniformName, float value) {
	glUniform1f(GetUniformLocation(uniformName), value);
}

void Shader::SetVec2(const std::string& uniformName, glm::vec2 value) {
	glUniform2fv(GetUniformLocation(uniformName), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& uniformName, glm::vec3 value) {
	glUniform3fv(GetUniformLocation(uniformName), 1, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string& uniformName, glm::vec4 value) {
	glUniform4fv(GetUniformLocation(uniformName), 1, glm::value_ptr(value));
}

void Shader::SetMat3(const std::string& uniformName, glm::mat3 value) {
	glUniformMatrix3fv(GetUniformLocation(uniformName), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& uniformName, glm::mat4 value) {
	glUniformMatrix4fv(GetUniformLocation(uniformName), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4Array(const std::string& uniformName, const std::vector<glm::mat4>& values) {
	if (values.empty()) {
		return;
	}

	glUniformMatrix4fv(GetUniformLocation(uniformName), static_cast<GLsizei>(values.size()), GL_FALSE, glm::value_ptr(values[0]));
}