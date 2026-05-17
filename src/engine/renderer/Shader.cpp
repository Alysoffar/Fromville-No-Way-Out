#include "engine/renderer/Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string& shaderName)
	: programID(0), name(shaderName) {}

std::string Shader::LoadFile(const std::string& path) {
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Shader '" + name + "' could not open file: " + path);
	}

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

GLuint Shader::CompileShader(GLenum type, const std::string& source) {
	const GLuint shader = glCreateShader(type);
	const char* sourcePointer = source.c_str();
	glShaderSource(shader, 1, &sourcePointer, nullptr);
	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE) {
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(static_cast<std::size_t>(logLength), '\0');
		glGetShaderInfoLog(shader, logLength, nullptr, log.data());
		glDeleteShader(shader);
		throw std::runtime_error("Shader '" + name + "' compilation failed: " + log);
	}

	return shader;
}

void Shader::Load(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath) {
	if (programID != 0) {
		glDeleteProgram(programID);
		programID = 0;
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

	uniformCache.clear();
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