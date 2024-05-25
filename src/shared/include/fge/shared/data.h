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

struct VariableDescription {
	uint size;
};

using ParameterDescriptions = std::vector<QString>;
using StateDescriptions = std::map<QString,VariableDescription>;

using ParameterBindings = std::map<QString, std::vector<C>>;

ParameterDescriptions descrFromParameters( const ParameterBindings& parameters );
