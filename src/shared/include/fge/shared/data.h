#pragma once

using uint = unsigned int;

struct SamplingSettings {
	uint resolution = 0;
	uint interpolation = 1;
	bool caching = false;
};
