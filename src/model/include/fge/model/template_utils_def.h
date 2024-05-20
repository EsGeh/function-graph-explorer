#pragma once

#include "fge/model/template_utils.h"
#include <type_traits>

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
			IsSetter<function>::value,
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
}

using ModelImpl = ScheduledFunctionCollectionImpl;
