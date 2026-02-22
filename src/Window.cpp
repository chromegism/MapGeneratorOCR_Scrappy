#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Window.h"

void Window::init() {
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == false) {
		throw std::runtime_error("Error initialising SDL3: " + (std::string)SDL_GetError());
	} // Automatically initialises SDL_INIT_EVENTS

	handle = SDL_CreateWindow("Erosion test", 1280, 720, SDL_WINDOW_VULKAN);
	if (!handle) {
		throw std::runtime_error("Error creating window: " + (std::string)SDL_GetError());
	}
}

void Window::kill() {
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_DestroyWindow(handle);
}

Window::~Window() {
	kill();
}