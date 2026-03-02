#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Window.h"

void Window::init() {
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == false) {
		throw std::runtime_error("Error initialising SDL3: " + (std::string)SDL_GetError());
	} // Automatically initialises SDL_INIT_EVENTS

	handle_ = SDL_CreateWindow(details_.name.c_str(), details_.width, details_.height, SDL_WINDOW_VULKAN);
	if (!handle_) {
		throw std::runtime_error("Error creating window: " + (std::string)SDL_GetError());
	}
}