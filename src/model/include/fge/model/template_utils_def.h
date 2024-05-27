#pragma once

#include "fge/model/template_utils.h"
#include "sampled_func_collection.h"
#include "sampled_func_collection_impl.h"
#include <memory>
#include <type_traits>
#include <QDebug>
#include <unistd.h>

// utils:

template <typename T, typename Tuple>
struct tuple_contains;

template <typename T, typename... Us>
struct tuple_contains<T, std::tuple<Us...>>
	: std::disjunction<std::is_same<T, Us>...>
{};

template <auto function>
struct IsSetterFunction : tuple_contains<
		std::pair<decltype(function), const char*>,
		std::remove_cvref_t<decltype(setters)>
>
{};

template <typename Setter>
struct IsSetter : std::false_type {};

template <auto function>
struct IsSetter< SetterTask<function> >
	: IsSetterFunction<function>
{};

template <typename T>
struct SetterTraits;

template <auto function>
struct SetterTraits
<SetterTask<function>>
{
	using type = decltype(function);
	static constexpr decltype(function) value = function;
};

template <typename Function>
const char* functionName() {
	return std::get<std::pair<Function,const char*>>( setters ).second;
};

template <auto function>
const char* functionName(const SetterTask<function>& setterTask) {
	using Function = typename SetterTraits<SetterTask<function>>::type;
	return functionName<Function>();
};

// definitions:

template <auto function>
struct SetterTask
{
	typename FunctionTraits<decltype(function)>::args_tuple_t args;
	PlaybackPosition pos = 0;
	constexpr static SampledFunctionCollectionImpl* temp = nullptr;
	using ReturnType = decltype(
		std::apply(
			function,
			std::tuple_cat(
				std::make_tuple(temp),
				args
			)
		)
	);
	std::promise<ReturnType> promise;
	bool done = false;
};

template <
	auto function,
	typename TaskQueue,
	typename... Args
>
auto makeSetter(
		TaskQueue& taskQueue,
		const PlaybackPosition position,
		Args... args
)
{
	static_assert(
			IsSetterFunction<function>::value,
			"Not a valid setter function"
	);
	auto tuple = std::tuple( args... );
	auto task = SetterTask<function>{
		.args = tuple,
		.pos = position
	};
	auto future = task.promise.get_future();
	taskQueue.push_back(std::move(task));
	return future;
}

template <auto function>
void run(
		SampledFunctionCollectionImpl* network,
		SetterTask<function>* setter
)
{
	using Return = typename FunctionTraits<decltype(function)>::ret_t;
	if constexpr ( ! std::is_void<Return>::value ) {
		auto ret = std::apply(
				function,
				std::tuple_cat(
					std::make_tuple(network),
					setter->args
				)
		);
		setter->promise.set_value(ret);
	}
	else {
		std::apply(
				function,
				std::tuple_cat(
					std::make_tuple(network),
					setter->args
				)
		);
		setter->promise.set_value();
	}
	setter->done = true;
}

using ModelImpl = ScheduledFunctionCollectionImpl;
