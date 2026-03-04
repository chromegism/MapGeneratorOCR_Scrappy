#include "pch.h"

#include "Perlin.h"

constexpr float PI = 3.14159265358f;

#if defined(_MSC_VER)
	#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
	#include <cpuid.h>
#endif

bool check_avx_support() {
	int cpuInfo[4];

	// 1. Check for CPUID support and get max supported leaf
#if defined(_MSC_VER)
	__cpuid(cpuInfo, 0);
#else
	if (__get_cpuid_max(0, nullptr) < 1) return false;
	__cpuid(0, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

	// 2. Check for AVX feature bit (Leaf 1, ECX bit 28)
	// and OSXSAVE bit (Leaf 1, ECX bit 27)
#if defined(_MSC_VER)
	__cpuid(cpuInfo, 1);
#else
	__cpuid(1, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

	bool cpu_has_avx = (cpuInfo[2] & (1 << 28)) != 0;
	bool os_has_xsave = (cpuInfo[2] & (1 << 27)) != 0;

	if (cpu_has_avx && os_has_xsave) {
		// 3. Check if OS actually enabled AVX via XGETBV
		// We check XCR0 (Extended Control Register 0)
		// Bit 1 = SSE state, Bit 2 = AVX state. Both must be 1 (0x6).
#if defined(_MSC_VER)
		unsigned __int64 xcr0 = _xgetbv(0);
#else
		unsigned int eax, edx;
		__asm__ volatile("xgetbv" : "=a" (eax), "=d" (edx) : "c" (0));
		unsigned long long xcr0 = ((unsigned long long)edx << 32) | eax;
#endif
		return (xcr0 & 0x6) == 0x6;
	}

	return false;
}

constexpr size_t SIMD_WIDTH = 8; // AVX2 256-bit floats -> 8 floats

std::random_device rd;
std::mt19937 gen{ rd() };
std::uniform_real_distribution<float> dist{ -PI, PI };

inline std::vector<float> make_xs(uint32_t width, uint32_t height) {
	std::vector<float> arr(width * height);
	float inv_width = 1.f / width;

	// fill first block
	for (size_t i = 0; i < width; ++i)
		arr[i] = i * inv_width;

	// repeat m times
	for (size_t j = 1; j < height; ++j)
		std::memcpy(&arr[j * width], &arr[0], width * sizeof(float));

	return arr;
}

inline std::vector<float> make_ys(uint32_t width, uint32_t height, uint32_t max_height) {
	std::vector<float> arr(width * height);
	float inv_width = 1.f / max_height;

	for (size_t j = 0; j < height; ++j)
		std::fill(arr.begin() + j * width, arr.begin() + (j + 1) * width, j * inv_width);

	return arr;
}

PerlinMap::PerlinLayer::PerlinLayer(float x_multiplier, float y_multiplier, float octave) {
	size_x = octave * x_multiplier;
	size_y = octave * y_multiplier;

	arrows_x = (unsigned int)ceil(size_x);
	arrows_y = (unsigned int)ceil(size_y);

	arrows = ArrowGrid::random(arrows_x * arrows_y);
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
	layers.reserve(details.octaves.size());
	for (float octave : details.octaves) {
		layers.emplace_back(PerlinLayer(details.x_multiplier, details.y_multiplier, octave));
	}
}


Arrow2D Arrow2D::random() {
	const float angle = dist(gen);
	return { std::cos(angle), std::sin(angle) };
}

ArrowGrid ArrowGrid::random(size_t size) {
	ArrowGrid grid;
	grid.xs.resize(size);
	grid.ys.resize(size);

	for (size_t i = 0; i < size; i++) {
		const float angle = dist(gen);
		grid.xs.at(i) = std::cos(angle);
		grid.ys.at(i) = std::sin(angle);
	}

	return grid;
}

inline float Arrow2D_dot(const Arrow2D& A, const Arrow2D& B) noexcept {
	return A.x * B.x + A.y * B.y;
}

inline float raw_dot(const float Ax, const float Ay, const float Bx, const float By) noexcept {
	return Ax * Bx + Ay * By;
}

inline __m256 raw_dot_m256(const __m256 Ax, const __m256 Ay, const __m256 Bx, const __m256 By) noexcept {
	return _mm256_add_ps(_mm256_mul_ps(Ax, Bx), _mm256_mul_ps(Ay, By));
}

inline float half_raw_dot(const float Ax, const float Ay, const Arrow2D& B) noexcept {
	return Ax * B.x + Ay * B.y;
}

inline float lerp(float v1, float v2, float x) noexcept  {
	return v1 + x * (v2 - v1);
}

inline __m256 lerp_m256(const __m256 v1, const __m256 v2, const __m256 x) noexcept {
	return _mm256_add_ps(v1, _mm256_mul_ps(x, _mm256_sub_ps(v2, v1)));
}

inline float fade(float t) noexcept {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

inline __m256 fade_m256(const __m256 t) noexcept {
	const __m256 t2 = _mm256_mul_ps(t, t);
	const __m256 t3 = _mm256_mul_ps(t2, t);
	__m256 tmp = _mm256_fmsub_ps(t, _mm256_set1_ps(6.f), _mm256_set1_ps(15.f));
	tmp = _mm256_fmadd_ps(t, tmp, _mm256_set1_ps(10.f));
	return _mm256_mul_ps(t3, tmp);
}

inline float smoothstep(float v1, float v2, float x) noexcept {
	return lerp(v1, v2, fade(x));
}

inline __m256 smoothstep_m256(const __m256 v1, const __m256 v2, const __m256 x) noexcept {
	return lerp_m256(v1, v2, fade_m256(x));
}

float PerlinMap::PerlinLayer::index(float x, float y) const {
	const float grid_x = x * (size_x - 1);
	const float grid_y = y * (size_y - 1);

	const unsigned int cell_x = (unsigned int)(grid_x);
	const unsigned int cell_y = (unsigned int)(grid_y);

	const float local_x = grid_x - cell_x;
	const float local_y = grid_y - cell_y;

	const float fx = fade(local_x);

	const unsigned int row0 = cell_y * arrows_x;
	const unsigned int row1 = row0 + arrows_x;

	const unsigned int i00 = row0 + cell_x;
	const unsigned int i10 = i00 + 1;
	const unsigned int i01 = row1 + cell_x;
	const unsigned int i11 = i01 + 1;

	const float s1 = lerp(
		raw_dot(local_x, local_y, arrows.xs[i00], arrows.ys[i00]),
		raw_dot(local_x - 1.f, local_y, arrows.xs[i10], arrows.ys[i10]),
		fx
	);
	const float s2 = lerp(
		raw_dot(local_x, local_y - 1.f, arrows.xs[i01], arrows.ys[i01]),
		raw_dot(local_x - 1.f, local_y - 1.f, arrows.xs[i11], arrows.ys[i11]),
		fx
	);

	return smoothstep(s1, s2, local_y);
}

inline void PerlinMap::PerlinLayer::batch_index_simd(const float* xs, const float* ys, const float y_offset, float* out, __m256 multiplier) const {
	const __m256i unit_epi32 = _mm256_set1_epi32(1);
	const __m256 unit_ps = _mm256_set1_ps(1.f);
	const __m256 unitn_ps = _mm256_set1_ps(-1.f);

	const __m256 grid_xs = _mm256_mul_ps( _mm256_loadu_ps(xs), _mm256_set1_ps(size_x - 1) );
	const __m256 acc_ys = _mm256_add_ps(_mm256_loadu_ps(ys), _mm256_set1_ps(y_offset));
	const __m256 grid_ys = _mm256_mul_ps( acc_ys, _mm256_set1_ps(size_y - 1) );

	const __m256i cell_xs = _mm256_cvttps_epi32(grid_xs);
	const __m256i cell_ys = _mm256_cvttps_epi32(grid_ys);

	// _mm256_cvtepi32_ps is epi32 to ps
	const __m256 local_xs = _mm256_sub_ps(grid_xs, _mm256_cvtepi32_ps(cell_xs));
	const __m256 local_ys = _mm256_sub_ps(grid_ys, _mm256_cvtepi32_ps(cell_ys));

	const __m256 local_x_sub1s = _mm256_sub_ps(local_xs, unit_ps);
	const __m256 local_y_sub1s = _mm256_sub_ps(local_ys, unit_ps);

	const __m256 fxs = fade_m256(local_xs);

	const __m256i row0s = _mm256_mullo_epi32(cell_ys, _mm256_set1_epi32(arrows_x));
	const __m256i row1s = _mm256_add_epi32(row0s, _mm256_set1_epi32(arrows_x));

	const __m256i i00s = _mm256_add_epi32(row0s, cell_xs);
	const __m256i i10s = _mm256_add_epi32(i00s, unit_epi32);
	const __m256i i01s = _mm256_add_epi32(row1s, cell_xs);
	const __m256i i11s = _mm256_add_epi32(i01s, unit_epi32);

	// sizeof(float) = 4
	const __m256 arrow_x_00s = _mm256_i32gather_ps(arrows.xs.data(), i00s, sizeof(float));
	const __m256 arrow_y_00s = _mm256_i32gather_ps(arrows.ys.data(), i00s, sizeof(float));
	const __m256 arrow_x_10s = _mm256_i32gather_ps(arrows.xs.data(), i10s, sizeof(float));
	const __m256 arrow_y_10s = _mm256_i32gather_ps(arrows.ys.data(), i10s, sizeof(float));
	const __m256 arrow_x_01s = _mm256_i32gather_ps(arrows.xs.data(), i01s, sizeof(float));
	const __m256 arrow_y_01s = _mm256_i32gather_ps(arrows.ys.data(), i01s, sizeof(float));
	const __m256 arrow_x_11s = _mm256_i32gather_ps(arrows.xs.data(), i11s, sizeof(float));
	const __m256 arrow_y_11s = _mm256_i32gather_ps(arrows.ys.data(), i11s, sizeof(float));

	const __m256 s1s = lerp_m256(
		raw_dot_m256(local_xs, local_ys, arrow_x_00s, arrow_y_00s),
		raw_dot_m256(local_x_sub1s, local_ys, arrow_x_10s, arrow_y_10s),
		fxs
	);
	const __m256 s2s = lerp_m256(
		raw_dot_m256(local_xs, local_y_sub1s, arrow_x_01s, arrow_y_01s),
		raw_dot_m256(local_x_sub1s, local_y_sub1s, arrow_x_11s, arrow_y_11s),
		fxs
	);

	const __m256 prev = _mm256_loadu_ps(out);

	__m256 vs = smoothstep_m256(s1s, s2s, local_ys);
	vs = _mm256_mul_ps(vs, multiplier);
	vs = _mm256_add_ps(vs, prev);
	_mm256_storeu_ps(out, vs);
}

float PerlinMap::index(float x, float y) const {
	float value = 0;

	float invAmplitude = 1;

	for (auto& layer : layers) {
		value += layer.index(x, y) * invAmplitude;
		invAmplitude /= details.base;
	}

	return std::min(std::max(value, -1.f), 1.f);
}

inline void PerlinMap::batch_index_simd(const float* xs, const float* ys, const float y_offset, float* out, size_t count) const {
	// redeclaration due to architecture support
	const __m256i unit_epi32 = _mm256_set1_epi32(1);
	const __m256 unit_ps = _mm256_set1_ps(1.f);
	const __m256 unitn_ps = _mm256_set1_ps(-1.f);

	std::fill(out, out + count, 0.0f);

	float invAmplitude = 1;
	__m256 base_m256 = _mm256_set1_ps(details.base);

	for (auto& layer : layers) {
		// same size as a float
		size_t i = 0;
		__m256 invAmplitude_m256 = _mm256_set1_ps(invAmplitude);

		// Process vectorised ones, SIMD_WIDTH at once
		for (; i + SIMD_WIDTH <= count; i += SIMD_WIDTH) {
			layer.batch_index_simd(xs + i, ys + i, y_offset, out + i, invAmplitude_m256);
			const __m256 prev = _mm256_loadu_ps(out + i);
			const __m256 val = _mm256_min_ps(_mm256_max_ps(prev, unitn_ps), unit_ps);
			_mm256_store_ps(out + i, val);
		}

		// Process others
		for (; i < count; i++) {
			float v = layer.index(xs[i], ys[i] + y_offset);
			out[i] = v * invAmplitude;
		}

		invAmplitude /= details.base;
	}
}

void PerlinMap::PerlinLayer::clear() {
	arrows.clear();
}

void PerlinMap::clear() {
	for (auto& layer : layers) {
		layer.clear();
	}
}

void PerlinMap::genInto(float* buffer, uint32_t width, uint32_t height) const {
	const float invW = 1.0f / width;
	const float invH = 1.0f / height;

	uint32_t threadCount = std::thread::hardware_concurrency();
	if (threadCount == 0) threadCount = 4;

	std::vector<std::thread> threads;
	threads.reserve(threadCount);

	uint32_t rowsPerThread = height / threadCount;

	if (check_avx_support()) {
		std::vector<float> xs = make_xs(width, rowsPerThread);
		std::vector<float> ys = make_ys(width, rowsPerThread, height);

		for (uint32_t t = 0; t < threadCount; t++) {
			uint32_t startRow = t * rowsPerThread;
			uint32_t endRow = (t == threadCount - 1)
				? height
				: startRow + rowsPerThread;

			threads.emplace_back([&, startRow, endRow]() {

				batch_index_simd(xs.data(), ys.data(), startRow * invH, buffer + startRow * width, rowsPerThread * width);

				});
		}

		// Required to be in here due to lifetimes
		for (auto& th : threads)
			th.join();
	}
	else {
		for (uint32_t t = 0; t < threadCount; t++) {
			uint32_t startRow = t * rowsPerThread;
			uint32_t endRow = (t == threadCount - 1)
				? height
				: startRow + rowsPerThread;

			threads.emplace_back([&, startRow, endRow]() {
				for (uint32_t y = startRow; y < endRow; y++) {
					const float fy = y * invH;

					for (uint32_t x = 0; x < width; x++) {
						buffer[x + y * width] =
							index(x * invW, fy);
					}
				}
			});
		}

		for (auto& th : threads)
			th.join();
	}
}