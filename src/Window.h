#pragma once

#include <string>
#include <cstdint>

struct SDL_Window;

class Window {
private:
	SDL_Window* handle_ = nullptr;

	void init();

public:
	struct Details {
		std::string name;
		uint32_t width, height;
	};
private:
	Details details_;

public:
	Window() noexcept = default;
	Window(const Details& _details) : details_(_details) { init(); }
	Window(const std::string& _name, uint32_t _width, uint32_t _height) : details_({ _name, _width, _height }) { init(); }
	~Window() { destroy(); }

	Window(const Window&) noexcept = delete; // move only
	Window(Window&& other) noexcept : handle_(other.handle_) {
		other.handle_ = nullptr;
	}
	Window& operator=(Window&& other) noexcept {
		if (this != &other) {
			if (handle_)
				destroy();

			handle_ = other.handle();
			other.handle_ = nullptr;
		}
		return *this;
	}

	SDL_Window* handle() const noexcept { return handle_; }
	bool isValid() const noexcept { return handle_ != nullptr; }
	const Details& details() const noexcept { return details_; }

	void destroy() {
		if (isValid()) {
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			SDL_DestroyWindow(handle_);
			handle_ = nullptr;
		}
	}
};