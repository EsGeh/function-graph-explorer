#pragma once

#include "fge/shared/complex_adaptor.h"
#include <QString>
#include <expected>

typedef cmplx::complex_t C;
typedef double T;


typedef QString Error;

template <typename T>
using ErrorOrValue = std::expected<T, Error>;

typedef std::optional<Error> MaybeError;

QString to_qstring(const C& value);
QString to_qstring(const std::pair<C,C>& value);
