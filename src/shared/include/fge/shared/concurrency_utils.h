#pragma once
#include <concepts>
#include <optional>
#include <mutex>
#include <QDebug>
#include <thread>
#include <type_traits>

#ifdef DEBUG_CONCURRENCY
#define LOG_MUTEX
#endif

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
  mutex_guarded(const QString& name)
		:name(name) {}
  explicit mutex_guarded(T in, const QString& name)
		: data(std::move(in))
		, name(name)
	{}

	// READ / WRITE data:
  auto read( auto f ) const {
		LOG_MUTEX_GUARDED( "READ" );
		if constexpr ( std::is_same_v<decltype(f(data)), void> ) {
			{
				auto l = lock();
				LOG_MUTEX_GUARDED( "READ", "acquired" );
				f(data);
			}
			LOG_MUTEX_GUARDED( "READ", "released" );
			return;
		}
		else {
			auto ret = [this,f]{
				auto l = lock();
				LOG_MUTEX_GUARDED( "READ", "acquired" );
				return f(data);
			}();
			LOG_MUTEX_GUARDED( "READ", "released" );
			return ret;
		}
  }
  auto write( auto f ) {
		LOG_MUTEX_GUARDED( "WRITE" );
		if constexpr ( std::is_same_v<decltype(f(data)), void> ) {
			{
				auto l = lock();
				LOG_MUTEX_GUARDED( "WRITE", "acquired" );
				f(data);
			}
			LOG_MUTEX_GUARDED( "WRITE", "released" );
			return;
		}
		else {
			auto ret = [this,f]{
				auto l = lock();
				LOG_MUTEX_GUARDED( "WRITE", "acquired" );
				return f(data);
			}();
			LOG_MUTEX_GUARDED( "WRITE", "released" );
			return ret;
		}
  }
  auto try_read( auto f ) const
	{
		LOG_MUTEX_GUARDED( "TRY READ" );
    auto l = try_lock();
		if constexpr ( std::is_same_v<decltype(f(data)), void> ) {
			if( l ) {
				LOG_MUTEX_GUARDED( "TRY READ", "acquired");
				f(data);
				LOG_MUTEX_GUARDED( "TRY READ", "released");
				l.unlock();
				return;
			}
			LOG_MUTEX_GUARDED( "TRY READ", "give up");
		}
		else {
			if( l ) {
				LOG_MUTEX_GUARDED( "TRY READ", "acquired");
				auto ret = std::make_optional(f(data));
				LOG_MUTEX_GUARDED( "TRY READ", "released");
				l.unlock();
				return ret;
			}
			LOG_MUTEX_GUARDED( "TRY READ", "give up");
			return std::optional<decltype(f(data))>{};
		}
  }
  auto try_write( auto f )
	{
		LOG_MUTEX_GUARDED( "TRY WRITE" );
    auto l = try_lock();
		if constexpr ( std::is_same_v<decltype(f(data)), void> ) {
			if( l ) {
				LOG_MUTEX_GUARDED( "TRY WRITE", "acquired" );
				f(data);
				LOG_MUTEX_GUARDED( "TRY WRITE", "released");
				l.unlock();
				return;
			}
			LOG_MUTEX_GUARDED( "TRY WRITE", "give up" );
		}
		else {
			if( l ) {
				LOG_MUTEX_GUARDED( "TRY WRITE", "acquired" );
				auto ret = std::make_optional(f(data));
				LOG_MUTEX_GUARDED( "TRY WRITE", "released");
				l.unlock();
				return ret;
			}
			LOG_MUTEX_GUARDED( "TRY WRITE", "give up" );
			return std::optional<decltype(f(data))>{};
		}
  }
private:
  mutable M m;
  T data;
	const QString name;
private:
  auto lock() const {
		return RL<M>(m);
	}
  auto lock() {
		return WL<M>(m);
	}
  auto try_lock() const { return RL<M>(m, std::try_to_lock); }
  auto try_lock() { return WL<M>(m, std::try_to_lock); }
	void LOG_MUTEX_GUARDED(const QString& op, const QString& msg = "") const
	{
#ifdef LOG_MUTEX
		if( msg == "" ) {
			qDebug() << QString("%2 %3 %1")
				.arg( name )
				.arg( to_qstring( std::this_thread::get_id() ) )
				.arg( op )
			;
		}
		else {
			qDebug() << QString("%2 %3 %1: %4")
				.arg( name )
				.arg( to_qstring( std::this_thread::get_id() ) )
				.arg( op )
				.arg( msg )
			;
		}
#endif
	}
};
