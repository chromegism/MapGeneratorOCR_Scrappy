#include <iostream>

#include <SDL3/SDL.h>

#include "App.h"

void Camera::forward(float by) {
	const glm::vec3 mov = orientation * glm::vec3(0, 0, -1);
	pos += mov;
}
void Camera::backward(float by) {
	const glm::vec3 mov = orientation * glm::vec3(0, 0, 1);
	pos += mov;
}
void Camera::left(float by) {
	const glm::vec3 mov = orientation * glm::vec3(-1, 0, 0);
	pos += mov;
}
void Camera::right(float by) {
	const glm::vec3 mov = orientation * glm::vec3(1, 0, 0);
	pos += mov;
}
void Camera::up(float by) {
	const glm::vec3 mov = orientation * glm::vec3(0, 1, 0);
	pos += mov;
}
void Camera::down(float by) {
	const glm::vec3 mov = orientation * glm::vec3(0, -1, 0);
	pos += mov;
}

void Application::init() {
	camera.orientation = glm::quatLookAt(glm::vec3(1, 0, 0) , glm::vec3(0, 1, 0));

	window.init();
	terrainGenerator.updateDetails(settings);
	renderer.init(window.getHandle(), terrainGenerator);

	lastFrameTime = Clock::now();
}

void Application::run() {
	running = true;

	uint32_t frames = 0;
	while (running) {
		update();
		renderer.drawFrame();
		frames++;
	}
	std::cout << frames << std::endl;
	renderer.waitIdle();
}

void Application::pollEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_EVENT_QUIT:     running = false; return;
			case SDL_EVENT_KEY_DOWN: keyState[event.key.scancode] = true; continue;
			case SDL_EVENT_KEY_UP:   keyState[event.key.scancode] = false; continue;
			default: return;
		}
	}
}

void Application::updateCamera() {
	if (keyState[SDL_SCANCODE_W]) {
		camera.forward(1);
	}
	if (keyState[SDL_SCANCODE_S]) {
		camera.backward(1);
	}
	if (keyState[SDL_SCANCODE_A]) {
		camera.left(1);
	}
	if (keyState[SDL_SCANCODE_D]) {
		camera.right(1);
	}
	if (keyState[SDL_SCANCODE_C]) {
		camera.down(1);
	}
	if (keyState[SDL_SCANCODE_SPACE]) {
		camera.up(1);
	}
}

void Application::updateDeltaTime() {
	Clock::time_point thisFrameTime = Clock::now();
	std::chrono::duration<float> delta = thisFrameTime - lastFrameTime;
	deltaTime = delta.count();
	lastFrameTime = thisFrameTime;
}

void Application::update() {
	updateDeltaTime();
	timeSinceMapUpdate += deltaTime;
	pollEvents();
	updateCamera();

	if (keyState[SDL_SCANCODE_RETURN]) {
		if (timeSinceMapUpdate >= mapUpdateCooldown) {
			std::cout << "Regenerating" << std::endl;
			auto before = Clock::now();
			renderer.updateTerrain(terrainGenerator);
			auto after = Clock::now();
			auto duration = after - before;
			std::cout << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(after - before).count() << "um" << std::endl;
			timeSinceMapUpdate = 0;
		}
	}
}