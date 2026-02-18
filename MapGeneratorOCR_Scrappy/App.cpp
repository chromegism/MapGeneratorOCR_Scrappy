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

	while (running) {
		pollEvents();
		update();
		renderer.drawFrame();
	}
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