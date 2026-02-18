#pragma once

struct SDL_Window;

class Window {
public:
	void init();
	void kill();

	~Window();

	SDL_Window* getHandle() { return handle; }
private:
	SDL_Window* handle;
};