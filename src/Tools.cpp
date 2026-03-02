#include "pch.h"

#include "Tools.h"

std::vector<const char*> stringVectorToCStrVector(const std::vector<std::string>& strings) {
	std::vector<const char*> cstrings{};
	cstrings.reserve(strings.size());

	std::ranges::copy(
		std::views::transform(strings, [](const std::string& s) { return s.c_str(); }),
		std::back_inserter(cstrings)
	);

	return cstrings;
}