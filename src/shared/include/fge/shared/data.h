#pragma once

#include <map>
#include <QString>
#include "fge/shared/complex_adaptor.h"

using uint = unsigned int;

typedef cmplx::complex_t C;
typedef double T;

struct SamplingSettings {
	uint resolution = 0;
	uint interpolation = 1;
	T periodic = 1; // 0 means: not periodic
	bool buffered = false;
};

struct ParameterDescription {
	double initial = 0;
	double min = 0;
	double max = 1;
	double step = 0;
};

using ParameterDescriptions = std::map<QString,ParameterDescription>;
using ParameterNames = std::vector<QString>;
using ParameterBindings = std::map<QString, std::vector<C>>;

struct StateDescription {
	uint size;
};
using StateDescriptions = std::map<QString,StateDescription>;
