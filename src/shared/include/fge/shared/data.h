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
	bool caching = false;
};

using ParameterDescription = std::vector<QString>;

using ParameterBindings = std::map<QString, C>;

std::vector<QString> descrFromParameters( const ParameterBindings& parameters );
