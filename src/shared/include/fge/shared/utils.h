#pragma once
#include "fge/shared/data.h"
#include <thread>
#include <mutex>
#include <utility>

QString to_qstring(const C& value);
QString to_qstring(const std::pair<C,C>& value);

QString to_qstring(const std::thread::id);
