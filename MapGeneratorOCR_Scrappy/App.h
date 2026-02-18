#pragma once

#include <atomic>

#include "Render.h"
#include "Window.h"

class Application {
public:
	void init();

	~Application();

	void run();

	void pollEvents();
	void update();

	std::atomic<bool> running;

private:
	Window window;
	Renderer renderer;
};