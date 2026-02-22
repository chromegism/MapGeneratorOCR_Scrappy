#pragma once

#include <atomic>
#include <map>
#include <chrono>

#include "Render.h"
#include "Window.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

using Clock = std::chrono::steady_clock;

struct Camera {
	glm::vec3 pos;
	glm::quat orientation;
	void forward(float by);
	void backward(float by);
	void left(float by);
	void right(float by);
	void up(float by);
	void down(float by);
};

class Application {
public:
	void init();

	~Application() = default;

	void run();

private:
	std::atomic<bool> running = false;

	Window window;
	Renderer renderer;
	Camera camera;
	std::map<SDL_Scancode, bool> keyState;

	Clock::time_point lastFrameTime;
	double deltaTime = 0;

	void updateCamera();
	void pollEvents();
	void updateDeltaTime();

	void update();
};