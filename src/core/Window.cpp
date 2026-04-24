#include "Window.h"

#include <iostream>
#include <cstdio>

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Window::Window(int widthValue, int heightValue, const std::string& windowTitle)
	: handle(nullptr), width(widthValue), height(heightValue), title(windowTitle), vsync(true) {}

Window::~Window() {
	if (handle) {
		glfwDestroyWindow(handle);
		handle = nullptr;
	}
	glfwTerminate();
}

bool Window::Init() {
	glfwSetErrorCallback(ErrorCallback);
	if (!glfwInit()) {
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!handle) {
		std::fprintf(stderr, "GLFW Error: failed to create window %s (%dx%d)\n", title.c_str(), width, height);
		return false;
	}

	std::printf("Window created: %s (%dx%d)\n", title.c_str(), width, height);

	glfwSetWindowPos(handle, 80, 80);
	glfwRestoreWindow(handle);
	glfwShowWindow(handle);
	glfwFocusWindow(handle);

	glfwMakeContextCurrent(handle);
	glfwSetWindowUserPointer(handle, this);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		return false;
	}

	glfwSwapInterval(vsync ? 1 : 0);
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	// Set winding order to CCW (default)
	glFrontFace(GL_CCW); 
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // Cull the back faces
	
	// glDisable(GL_CULL_FACE); // Uncomment to temporarily disable culling for debugging
	
	glEnable(GL_MULTISAMPLE);
	glfwSetFramebufferSizeCallback(handle, FramebufferSizeCallback);

	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	std::cout << "OpenGL Version: " << (version ? reinterpret_cast<const char*>(version) : "Unknown") << '\n';
	std::cout << "Renderer: " << (renderer ? reinterpret_cast<const char*>(renderer) : "Unknown") << '\n';
	std::cout << "Vendor: " << (vendor ? reinterpret_cast<const char*>(vendor) : "Unknown") << '\n';

	return true;
}

void Window::PollEvents() {
	glfwPollEvents();
}

void Window::SwapBuffers() {
	if (handle) {
		glfwSwapBuffers(handle);
	}
}

bool Window::ShouldClose() const {
	return handle ? glfwWindowShouldClose(handle) != GLFW_FALSE : true;
}

void Window::Close() {
	if (handle) {
		glfwSetWindowShouldClose(handle, GLFW_TRUE);
	}
}

void Window::SetVSync(bool enabled) {
	vsync = enabled;
	glfwSwapInterval(vsync ? 1 : 0);
}

float Window::GetAspectRatio() const {
	if (height == 0) {
		return 1.0f;
	}
	return static_cast<float>(width) / static_cast<float>(height);
}

int Window::GetWidth() const {
	return width;
}

int Window::GetHeight() const {
	return height;
}

GLFWwindow* Window::GetHandle() const {
	return handle;
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int w, int h) {
	auto* currentWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!currentWindow) {
		return;
	}

	currentWindow->width = w;
	currentWindow->height = h;
	glViewport(0, 0, w, h);
}

void Window::ErrorCallback(int error, const char* desc) {
	std::cerr << "GLFW Error " << error << ": " << (desc ? desc : "Unknown") << '\n';
}