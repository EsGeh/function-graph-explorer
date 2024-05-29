#pragma once
#include <concepts>
#include <optional>
#include <mutex>
#include <type_traits>

template<
	class T,
  class M=std::mutex,
  template<class...> class WL=std::unique_lock,
  template<class...> class RL=std::unique_lock
> 
struct mutex_guarded {

public:
	using This = mutex_guarded<T,M,WL,RL>;
	template <typename Return>
	struct ReturnType {
		using type = std::conditional<
			std::is_same<Return,void>::value,
			void,
			std::optional<Return>
		>::type;
	};

public:
	// constructors:
  mutex_guarded() = default;
  explicit mutex_guarded(T in) : data(std::move(in)) {}

	// READ / WRITE data:
  auto read( auto f ) const {
    auto l = lock();
    return f(data);
  }
  auto write( auto f ) {
    auto l = lock();
    return f(data);
  }
  auto try_read( auto f ) const
	{
    auto l = try_lock();
		if constexpr ( std::is_same_v<decltype(f(data)), void> ) {
			if( l ) {
				f(data);
				return;
			}
		}
		else {
			if( l ) {
				return std::make_optional(f(data));
			}
			return {};
		}
  }
  auto try_write( auto f )
	{
    auto l = try_lock();
		if constexpr ( std::is_same_v<decltype(f(data)), void> ) {
			if( l ) {
				f(data);
				return;
			}
		}
		else {
			if( l ) {
				return std::make_optional(f(data));
			}
			return {};
		}
  }
private:
  mutable M m;
  T data;
  auto lock() const { return RL<M>(m); }
  auto lock() { return WL<M>(m); }
  auto try_lock() const { return RL<M>(m, std::try_to_lock); }
  auto try_lock() { return WL<M>(m, std::try_to_lock); }
};
