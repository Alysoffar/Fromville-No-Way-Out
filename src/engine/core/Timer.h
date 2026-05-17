#pragma once

class Timer {
public:
	Timer();

	void Tick();
	float GetDeltaTime() const;
	float GetTotalTime() const;
	float GetFPS() const;

private:
	float deltaTime;
	float totalTime;
	float lastTime;
	bool firstTick;
	float frameTimes[60];
	int frameIndex;
	int frameCount;
	float frameTimeSum;
};