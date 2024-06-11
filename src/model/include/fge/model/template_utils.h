#pragma once
#include <tuple>
#include <type_traits>

// declarations:

class SampledFunctionCollectionImpl;

template <
	auto function
>
struct SetterTask;

template <typename Function>
struct FunctionTraits;

template <
	typename Return,
	typename... Args
> struct FunctionTraits
<Return (SampledFunctionCollectionImpl::*)(Args...)>
{
	using args_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
	using ret_t = Return;
};

using TaskDoneCallback = std::function<void()>;

template <
	auto function,
	typename TaskQueue,
	typename... Args
>
auto makeSetter(
		TaskQueue& taskQueue,
		const PlaybackPosition position,
		TaskDoneCallback taskDoneCallback,
		Args... args
);

template <auto function>
TaskDoneCallback run(
		SampledFunctionCollectionImpl* network,
		SetterTask<function>* setter
);
