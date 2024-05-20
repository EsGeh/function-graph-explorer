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

template <auto function>
const char* functionName() {
	return std::get<std::pair<decltype(function),const char*>>( setters ).second;
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
	setter->obj->writeTasks.pop_front();
	qDebug() << QString("executing '%1'").arg(
			functionName<function>()
	);
}

using ModelImpl = ScheduledFunctionCollectionImpl;
