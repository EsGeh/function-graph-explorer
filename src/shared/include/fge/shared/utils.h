#pragma once

#include "fge/shared/data.h"
#include <expected>
#include <thread>

typedef QString Error;

template <typename T>
using ErrorOrValue = std::expected<T, Error>;

typedef std::optional<Error> MaybeError;

QString to_qstring(const C& value);
QString to_qstring(const std::pair<C,C>& value);

QString to_qstring(const std::thread::id);
