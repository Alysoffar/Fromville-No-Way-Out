#pragma once

#include <string>

struct GLFWwindow;

class Window {
public:
	Window(int width, int height, const std::string& title);
	~Window();

	bool Init();
	void PollEvents();
	void SwapBuffers();
	bool ShouldClose() const;
	void SetVSync(bool enabled);
	float GetAspectRatio() const;
	int GetWidth() const;
	int GetHeight() const;
	GLFWwindow* GetHandle() const;

private:
	static void FramebufferSizeCallback(GLFWwindow* window, int w, int h);
	static void ErrorCallback(int error, const char* desc);

	GLFWwindow* handle;
	int width;
	int height;
	std::string title;
	bool vsync;
};