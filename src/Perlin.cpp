#include <cmath>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "Perlin.h"

constexpr float PI = 3.14159265358f;

std::random_device rd;
std::mt19937 gen{ rd() };
std::uniform_real_distribution<float> dist{ -PI, PI };

inline float clamp(float v, float lo, float hi) noexcept {
	return v < lo ? lo : (v > hi ? hi : v);
}

PerlinMap::PerlinLayer::PerlinLayer(float x_multiplier, float y_multiplier, float octave) {
	size_x = octave * x_multiplier;
	size_y = octave * y_multiplier;

	arrows_x = (unsigned int)ceil(size_x);
	arrows_y = (unsigned int)ceil(size_y);

	arrows.resize(arrows_x * arrows_y);

	for (unsigned int y = 0; y < arrows_y; y++) {
		for (unsigned int x = 0; x < arrows_x; x++) {
			const unsigned int index = y * arrows_x + x;
			arrows[index] = Arrow2D::random();
		}
	}
}

PerlinMap::PerlinMap(float xm, float ym, const std::vector<float> &octs, float b) {
	details.x_multiplier = xm;
	details.y_multiplier = ym;

	details.base = b;

	details.octaves = octs;

	regenerate();
}

PerlinMap::PerlinMap() {
	details.x_multiplier = 0;
	details.y_multiplier = 0;

	details.base = 0;

	details.octaves = {};
}

void PerlinMap::regenerate() {
	layers.clear();
	for (float octave : details.octaves) {
		layers.push_back(PerlinLayer(details.x_multiplier, details.y_multiplier, octave));
	}
}


Arrow2D Arrow2D::random() {
	const float angle = dist(gen);
	return { std::cos(angle), std::sin(angle) };
}

inline float Arrow2D::magnitude() const {
	return powf(x * x + y * y, 0.5f);
}

Arrow2D Arrow2D::normalise() const {
	float mag = magnitude();
	return { x / mag, y / mag };
}

inline float Arrow2D_dot(const Arrow2D& A, const Arrow2D& B) noexcept {
	return A.x * B.x + A.y * B.y;
}

inline float raw_dot(const float Ax, const float Ay, const float Bx, const float By) noexcept {
	return Ax * Bx + Ay * By;
}

inline float half_raw_dot(const float Ax, const float Ay, const Arrow2D& B) noexcept {
	return Ax * B.x + Ay * B.y;
}

inline float lerp(float v1, float v2, float x) noexcept  {
	return v1 + x * (v2 - v1);
}

inline float fade(float t) noexcept {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float smoothstep(float v1, float v2, float x) noexcept {
	return lerp(v1, v2, fade(x));
}

float PerlinMap::PerlinLayer::index(float x, float y) const {
	const float grid_x = x * (size_x - 1);
	const float grid_y = y * (size_y - 1);

	const unsigned int cell_x = (unsigned int)(grid_x);
	const unsigned int cell_y = (unsigned int)(grid_y);

	const float local_x = grid_x - cell_x;
	const float local_y = grid_y - cell_y;

	const float fx = fade(local_x);

	const unsigned int row = cell_y * arrows_x + cell_x;
	const unsigned int next_row = row + arrows_x;

	const float s1 = lerp(
		half_raw_dot(local_x, local_y, arrows[row]),
		half_raw_dot(local_x - 1.f, local_y, arrows[row + 1]),
		fx
	);
	const float s2 = lerp(
		half_raw_dot(local_x, local_y - 1.f, arrows[next_row]),
		half_raw_dot(local_x - 1.f, local_y - 1.f, arrows[next_row + 1]),
		fx
	);

	return smoothstep(s1, s2, local_y);

}

float PerlinMap::index(float x, float y) const {
	float value = 0;

	float invAmplitude = 1;

	for (auto& layer : layers) {
		value += layer.index(x, y) * invAmplitude;
		invAmplitude /= details.base;
	}

	return clamp(value, -1, 1);
}