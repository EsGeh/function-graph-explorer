#pragma once

#include "fge/model/template_utils.h"
#include <type_traits>
#include <QDebug>

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
	ScheduledFunctionCollectionImpl* obj;
	typename FunctionTraits<decltype(function)>::args_tuple_t args;
	PlaybackPosition pos = 0;
	using ReturnType = decltype(
			std::apply(function,std::tuple_cat(std::make_tuple(obj->getNetwork()),args))
	);
	std::promise<ReturnType> promise;
	bool done = false;
};

template <
	auto function,
	typename... Args
>
auto makeSetter(
		ScheduledFunctionCollectionImpl* obj,
		Args... args
)
{
	static_assert(
			IsSetterFunction<function>::value,
			"Not a valid setter function"
	);
	auto tuple = std::tuple( args... );
	auto task = SetterTask<function>{
		.obj = obj,
		.args = tuple,
		.pos = obj->position
	};
	auto future = task.promise.get_future();
	obj->writeTasks.push_back(std::move(task));
	return future;
}

template <auto function>
void run(
		SetterTask<function>* setter
)
{
	using Return = typename FunctionTraits<decltype(function)>::ret_t;
	if constexpr ( ! std::is_void<Return>::value ) {
		auto ret = std::apply(
				function,
				std::tuple_cat(
					std::make_tuple(setter->obj->getNetwork()),
					setter->args
				)
		);
		setter->promise.set_value(ret);
	}
	else {
		std::apply(
				function,
				std::tuple_cat(
					std::make_tuple(setter->obj->getNetwork()),
					setter->args
				)
		);
		setter->promise.set_value();
	}
	setter->done = true;
}

using ModelImpl = ScheduledFunctionCollectionImpl;
