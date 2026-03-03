#pragma once

#include <vector>

#include "immintrin.h"

struct Arrow2D {
	float x = 0.f;
	float y = 0.f;

	static Arrow2D random();
};
struct ArrowGrid {
	std::vector<float> xs;
	std::vector<float> ys;

	static ArrowGrid random(size_t size);
	void clear() { xs.clear(); ys.clear(); }
};

class PerlinMap {
private:
	struct PerlinLayer {
		ArrowGrid arrows;
		unsigned int arrows_x = 0;
		unsigned int arrows_y = 0;

		float size_x = 0.f;
		float size_y = 0.f;

		float octaves = 0.f;

		float index(float x, float y) const;
		void batch_index_simd(const float* xs, const float* ys, const float y_offset, float* out, __m256 multiplier) const;
		void clear();

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
	void clear();

	float index(float x, float y) const;
	void batch_index_simd(const float* xs, const float* ys, const float y_offset, float* out, size_t count) const;
};