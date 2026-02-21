#include <iostream>

#include <SDL3/SDL.h>

#include "App.h"

void Application::init() {
	window.init();
	renderer.init(window.getHandle());
}

Application::~Application() {
	window.~Window();
}

void Application::run() {
	running = true;

	uint32_t frames = 0;
	while (running) {
		pollEvents();
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
			case SDL_EVENT_QUIT: running = false; return;
			default: return;
		}
	}
}

void Application::update() {}