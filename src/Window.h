#pragma once

#include <string>
#include <cstdint>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_init.h>

struct SDL_Window;

class Window {
private:
	SDL_Window* handle_ = nullptr;

	void init() {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) == false) {
			throw std::runtime_error("Error initialising SDL3: " + (std::string)SDL_GetError());
		} // Automatically initialises SDL_INIT_EVENTS

		handle_ = SDL_CreateWindow(details_.name.c_str(), details_.width, details_.height, SDL_WINDOW_VULKAN);
		if (!handle_) {
			throw std::runtime_error("Error creating window: " + (std::string)SDL_GetError());
		}
	}

public:
	struct Details {
		std::string name;
		uint32_t width = 0, height = 0;
	};
private:
	Details details_;

public:
	Window() noexcept = default;
	Window(const Details& _details) : details_(_details) { init(); }
	Window(const std::string& _name, uint32_t _width, uint32_t _height) : details_({ _name, _width, _height }) { init(); }
	~Window() { destroy(); }

	Window(const Window&) noexcept = delete; // move only
	Window(Window&& other) noexcept : handle_(other.handle_), details_(other.details_) {
		other.handle_ = nullptr;
	}
	Window& operator=(Window&& other) noexcept {
		if (this != &other) {
			if (handle_)
				destroy();

			handle_ = other.handle();
			details_ = other.details();
			other.handle_ = nullptr;
		}
		return *this;
	}

	SDL_Window* handle() const noexcept { return handle_; }
	bool isValid() const noexcept { return handle_ != nullptr; }
	const Details& details() const noexcept { return details_; }

	void destroy() noexcept {
		if (isValid()) {
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			SDL_DestroyWindow(handle_);
			handle_ = nullptr;
		}
	}
};