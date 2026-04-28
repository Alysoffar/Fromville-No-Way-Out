#include "engine/core/InputManager.h"

#include <algorithm>

InputManager* InputManager::GetFromWindow(GLFWwindow* window) {
	return static_cast<InputManager*>(glfwGetWindowUserPointer(window));
}

void InputManager::SetWindow(GLFWwindow* win) {
	window = win;
	if (!window) {
		return;
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, &InputManager::KeyCallback);
	glfwSetMouseButtonCallback(window, &InputManager::MouseButtonCallback);
	glfwSetCursorPosCallback(window, &InputManager::CursorPosCallback);

	double x = 0.0;
	double y = 0.0;
	glfwGetCursorPos(window, &x, &y);
	previousMousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
	currentMousePos = previousMousePos;
}

void InputManager::Poll() {
	previousKeys = currentKeys;
	previousMousePos = currentMousePos;

	glfwPollEvents();

	if (window) {
		double x = 0.0;
		double y = 0.0;
		glfwGetCursorPos(window, &x, &y);
		currentMousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
	}

	mouseDelta = currentMousePos - previousMousePos;
}

bool InputManager::IsKeyDown(int glfwKey) const {
	if (glfwKey < 0 || glfwKey > GLFW_KEY_LAST) {
		return false;
	}
	return currentKeys[glfwKey] != 0;
}

bool InputManager::IsKeyPressed(int glfwKey) const {
	if (glfwKey < 0 || glfwKey > GLFW_KEY_LAST) {
		return false;
	}
	return currentKeys[glfwKey] != 0 && previousKeys[glfwKey] == 0;
}

bool InputManager::IsKeyReleased(int glfwKey) const {
	if (glfwKey < 0 || glfwKey > GLFW_KEY_LAST) {
		return false;
	}
	return currentKeys[glfwKey] == 0 && previousKeys[glfwKey] != 0;
}

glm::vec2 InputManager::GetMouseDelta() const {
	return mouseDelta;
}

glm::vec2 InputManager::GetMousePos() const {
	return currentMousePos;
}

bool InputManager::IsMouseButtonDown(int button) const {
	if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
		return false;
	}
	return currentButtons[button] != 0;
}

void InputManager::SetCursorLocked(bool locked) {
	if (!window) {
		return;
	}
	glfwSetInputMode(window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)scancode;
	(void)mods;

	InputManager* input = GetFromWindow(window);
	if (!input || key < 0 || key > GLFW_KEY_LAST) {
		return;
	}

	if (action == GLFW_PRESS) {
		input->currentKeys[key] = 1;
	} else if (action == GLFW_RELEASE) {
		input->currentKeys[key] = 0;
	}
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	(void)mods;

	InputManager* input = GetFromWindow(window);
	if (!input || button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
		return;
	}

	if (action == GLFW_PRESS) {
		input->currentButtons[button] = 1;
	} else if (action == GLFW_RELEASE) {
		input->currentButtons[button] = 0;
	}
}

void InputManager::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	InputManager* input = GetFromWindow(window);
	if (!input) {
		return;
	}
	input->currentMousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}