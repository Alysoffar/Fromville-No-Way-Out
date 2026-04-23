#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
public:
	explicit Shader(const std::string& name);

	void Load(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath = "");
	void Bind() const;
	void Unbind() const;
	GLuint GetID() const;

	void SetBool(const std::string& name, bool value);
	void SetInt(const std::string& name, int value);
	void SetFloat(const std::string& name, float value);
	void SetVec2(const std::string& name, glm::vec2 value);
	void SetVec3(const std::string& name, glm::vec3 value);
	void SetVec4(const std::string& name, glm::vec4 value);
	void SetMat3(const std::string& name, glm::mat3 value);
	void SetMat4(const std::string& name, glm::mat4 value);
	void SetMat4Array(const std::string& name, const std::vector<glm::mat4>& values);

private:
	GLuint CompileShader(GLenum type, const std::string& source);
	std::string LoadFile(const std::string& path);
	GLint GetUniformLocation(const std::string& uniformName);

	GLuint programID;
	std::string name;
	std::unordered_map<std::string, GLint> uniformCache;
};
