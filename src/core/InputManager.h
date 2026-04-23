#pragma once

#include <array>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class InputManager {
public:
	void Poll();
	bool IsKeyDown(int glfwKey) const;
	bool IsKeyPressed(int glfwKey) const;
	bool IsKeyReleased(int glfwKey) const;
	glm::vec2 GetMouseDelta() const;
	glm::vec2 GetMousePos() const;
	bool IsMouseButtonDown(int button) const;
	void SetWindow(GLFWwindow* win);
	void SetCursorLocked(bool locked);

private:
	GLFWwindow* window = nullptr;

	std::array<unsigned char, GLFW_KEY_LAST + 1> previousKeys{};
	std::array<unsigned char, GLFW_KEY_LAST + 1> currentKeys{};
	std::array<unsigned char, GLFW_MOUSE_BUTTON_LAST + 1> currentButtons{};

	glm::vec2 previousMousePos = glm::vec2(0.0f);
	glm::vec2 currentMousePos = glm::vec2(0.0f);
	glm::vec2 mouseDelta = glm::vec2(0.0f);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
	static InputManager* GetFromWindow(GLFWwindow* window);
};

