#pragma once
#include "fge/shared/data.h"
#include <expected>
#include <thread>
#include <mutex>

typedef QString Error;

template <typename T>
using ErrorOrValue = std::expected<T, Error>;

typedef std::optional<Error> MaybeError;

QString to_qstring(const C& value);
QString to_qstring(const std::pair<C,C>& value);

QString to_qstring(const std::thread::id);


template<
	class T,
  class M=std::mutex,
  template<class...> class WL=std::unique_lock,
  template<class...> class RL=std::unique_lock
> 
struct mutex_guarded {
  auto read( auto f ) const {
    auto l = lock();
    return f(t);
  }
  auto write( auto f ) {
    auto l = lock();
    return f(t);
  }
	T& raw() {
		return t;
	}
  mutex_guarded() = default;
  explicit mutex_guarded(T in) : t(std::move(in)) {}
private:
  mutable M m;
  T t;
  auto lock() const { return RL<M>(m); }
  auto lock() { return WL<M>(m); }
};
