#pragma once

#include <atomic>
#include <map>
#include <chrono>

#include "Render.h"
#include "Window.h"

#include "Terrain.h"
#include "MapSettings.h"

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
	MapSettings settings{ 1000, 1000, 1000, { 1.9f, 3.3f, 6.4f, 9.2f, 12.6f, 18.6f, 30.f, 60.f, 90.f }, 2 };
	TerrainGenerator terrainGenerator;
	std::map<SDL_Scancode, bool> keyState;

	const float mapUpdateCooldown = 0.2f; // In seconds
	float timeSinceMapUpdate = 0.f;

	Clock::time_point lastFrameTime;
	float deltaTime = 0;

	void updateCamera();
	void pollEvents();
	void updateDeltaTime();

	void update();
};