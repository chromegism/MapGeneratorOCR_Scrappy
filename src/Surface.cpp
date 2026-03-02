#include "pch.h"

#include "Surface.h"

void Surface::generate(SDL_Window* windowHandle) {
	std::cout << instanceHandle_ << std::endl;
	if (!SDL_Vulkan_CreateSurface(windowHandle, instanceHandle_, nullptr, &handle_)) {
		throw std::runtime_error("Failed to create Vulkan surface: " + (std::string)SDL_GetError());
	}
}