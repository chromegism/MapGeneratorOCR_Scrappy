#pragma once

#include <vector>
#include <random>

struct Arrow2D {
	float x = 0.f;
	float y = 0.f;

	static Arrow2D random();

	inline float magnitude() const;
	Arrow2D normalise() const;
};

class PerlinMap {
	struct PerlinLayer {
		std::vector<Arrow2D> arrows;
		unsigned int arrows_x = 0;
		unsigned int arrows_y = 0;

		float size_x = 0.f;
		float size_y = 0.f;

		float octaves = 0.f;

		float index(float x, float y) const;

		PerlinLayer(float x_multiplier, float y_multiplier, float octave);
	};

	std::vector<PerlinLayer> layers;

	struct {
		float x_multiplier = 0;
		float y_multiplier = 0;

		float base = 0;

		std::vector<float> octaves;
	} details;

public:
	PerlinMap(float x_multiplier, float y_multiplier, const std::vector<float>& octs, float b);
	PerlinMap();
	void regenerate();

	float index(float x, float y) const;
};