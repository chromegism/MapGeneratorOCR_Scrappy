#include <iostream>
#include <chrono>

#include <SDL3/SDL.h>

#include "MapSettings.h"
#include "MapGen.h"
#include "Terrain.h"

#include "App.h"

#include "stb_image_write.h"

int perlin_test() {
	std::srand((uint16_t)std::time(nullptr));

	MapSettings settings{ 4096, 4096, 1024, { 1.9f, 6.3f, 45.f, 108.f, 206.f, 402.f }, 2 };

	ValidateSettingsErrorCode error_code = validateSettings(settings);
	if (error_code != ValidateSettingsErrorCode::OK) {
		std::cerr << "Error: " << validateSettingsErrorCode_toString(error_code) << std::endl;
		exit(1);
	}

	auto start = std::chrono::steady_clock::now();

	Map map = genMap(settings);

	auto end = std::chrono::steady_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	std::cout << "Took " << ms << "ms\n";


	std::vector<unsigned char> image(settings.width * settings.height);

	for (int i = 0; i < settings.width * settings.height; i++) {
		const float value = map.terrain.heights[i];
		const unsigned char v = (unsigned char)((value * 0.5f + 0.5f) * 255.f);

		image[i] = v;
	}

	stbi_write_png("test.png", settings.width, settings.height, 1, image.data(), settings.width);

	std::cout << "Done!";
	std::cin.get();

	return EXIT_SUCCESS;
}

int render_test() {
	Application app;
	app.init();

	app.run();

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
	try {
		return render_test();
	}
	catch (std::exception& e) {
		std::cerr << "Caught exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}