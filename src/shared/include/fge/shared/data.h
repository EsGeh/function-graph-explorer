#pragma once

#include <map>
#include <QString>
#include <optional>
#include <expected>
#include "fge/shared/complex_adaptor.h"

using uint = unsigned int;

typedef cmplx::complex_t C;
typedef double T;

typedef QString Error;

template <typename T>
using ErrorOrValue = std::expected<T, Error>;

typedef std::optional<Error> MaybeError;

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

template <typename Value>
using VariableBindings = std::map<QString, Value>;

using ParameterBindings = VariableBindings<C>;
using StateBindings = VariableBindings<std::vector<C>>;

struct StateDescription {
	uint size;
};
using StateDescriptions = std::map<QString,StateDescription>;
