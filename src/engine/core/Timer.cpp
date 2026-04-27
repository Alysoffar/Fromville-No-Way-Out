#include "engine/core/Timer.h"

#include <algorithm>

#include <GLFW/glfw3.h>

Timer::Timer()
	: deltaTime(0.0f), totalTime(0.0f), lastTime(0.0f), firstTick(true), frameTimes{0.0f}, frameIndex(0), frameCount(0), frameTimeSum(0.0f) {}

void Timer::Tick() {
	const float currentTime = static_cast<float>(glfwGetTime());
	if (firstTick) {
		lastTime = currentTime;
		firstTick = false;
		deltaTime = 0.0f;
		return;
	}

	deltaTime = std::min(currentTime - lastTime, 0.05f);
	lastTime = currentTime;
	totalTime += deltaTime;

	frameTimeSum -= frameTimes[frameIndex];
	frameTimes[frameIndex] = deltaTime;
	frameTimeSum += deltaTime;
	frameIndex = (frameIndex + 1) % 60;
	if (frameCount < 60) {
		++frameCount;
	}
}

float Timer::GetDeltaTime() const {
	return deltaTime;
}

float Timer::GetTotalTime() const {
	return totalTime;
}

float Timer::GetFPS() const {
	if (frameCount == 0 || frameTimeSum <= 0.0f) {
		return 0.0f;
	}
	return static_cast<float>(frameCount) / frameTimeSum;
}