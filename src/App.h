#pragma once

#include <atomic>
#include <map>

#include "Render.h"
#include "Window.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

struct Camera {
	glm::vec3 pos;
	glm::quat orientation;
	void forward(float by);
	void backward(float by);
	void left(float by);
	void right(float by);
	void up(float by);
	void down(float by);
	void move(float by, glm::quat direction);
};

class Application {
public:
	void init();

	~Application() = default;

	void run();

private:
	std::atomic<bool> running;

	Window window;
	Renderer renderer;
	Camera camera;
	std::map<SDL_Scancode, bool> keyState;

	void updateCamera();
	void pollEvents();

	void update();
};