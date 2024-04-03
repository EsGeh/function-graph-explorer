#ifndef GLOBAL_H
#define GLOBAL_H

#include "complex_adaptor.h"
#include <QString>

typedef cmplx::complex_t C;
typedef double T;


typedef QString Error;

template <typename C>
using ErrorOrValue = std::variant<Error, C>;

typedef std::optional<Error> MaybeError;

#endif
