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
struct IsSetter : tuple_contains<
		std::pair<decltype(function), const char*>,
		std::remove_cvref_t<decltype(setters)>
>
{};

template <typename Setter>
struct IsSetterTask : std::false_type {};

template <auto function>
struct IsSetterTask< SetterTask<function> >
	: IsSetter<function>
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


namespace intern {
	template <typename Tuple, typename Function, int Index = 0>
	auto find(Tuple t, Function f)
		-> std::optional<const char*>
	{
		constexpr auto tup_size = std::tuple_size_v<Tuple>;

		if constexpr(Index == tup_size)
			return {};
		else {
			auto entry = std::get<Index>(t);
			if constexpr ( std::is_same_v<decltype(entry.first),Function> ) {
				if ( entry.first == f ) {
					return entry.second;
				}
				else {
					return find<Tuple, Function, 1+Index>(t, f);
				}
			} else {
				return find<Tuple, Function, 1+Index>(t, f);
			}
		}
	}
}

template <auto function>
const char* functionName(const SetterTask<function>& setterTask) {
	return intern::find( setters, function ).value();
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
	TaskDoneCallback taskDoneCallback = [](auto){};
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
		TaskDoneCallback taskDoneCallback,
		Args... args
)
{
	static_assert(
			IsSetter<function>::value,
			"Not a valid setter function"
	);
	auto tuple = std::tuple( args... );
	auto task = SetterTask<function>{
		.args = tuple,
		.pos = position,
		.taskDoneCallback = taskDoneCallback
	};
	auto future = task.promise.get_future();
	taskQueue.push_back(std::move(task));
	return future;
}

template <auto function>
TaskDoneCallback run(
		SampledFunctionCollectionImpl* network,
		SetterTask<function>* setter
)
{
	using Return = typename FunctionTraits<decltype(function)>::ret_t;
	TaskDoneCallback fillPromise;
	if constexpr ( ! std::is_void<Return>::value ) {
		auto ret = std::apply(
				function,
				std::tuple_cat(
					std::make_tuple(network),
					setter->args
				)
		);
		fillPromise = [setter,ret]{
			setter->promise.set_value(ret);
		};
	}
	else {
		std::apply(
				function,
				std::tuple_cat(
					std::make_tuple(network),
					setter->args
				)
		);
		fillPromise = [setter]{
			setter->promise.set_value();
		};
	}
	setter->done = true;
	return [fillPromise,setter]{
		fillPromise();
		setter->taskDoneCallback();
	};
}

using ModelImpl = ScheduledFunctionCollectionImpl;
