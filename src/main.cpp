#include <cstdio>

#include <glad/glad.h>

#include "core/Timer.h"
#include "core/Window.h"

int main() {
	Window window(1280, 720, "Fromville: No Way Out");
	if (!window.Init()) {
		std::fprintf(stderr, "Failed to initialize Fromville window\n");
		return 1;
	}

	Timer timer;
	while (!window.ShouldClose()) {
		timer.Tick();
		glClearColor(0.05f, 0.03f, 0.08f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		window.PollEvents();
		window.SwapBuffers();
	}

	return 0;
}
