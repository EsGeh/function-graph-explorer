#pragma once
#include <tuple>
#include <type_traits>

// declarations:

class SampledFunctionCollectionImpl;

template <auto function>
struct IsSetter : std::false_type {};

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

template <
	auto function,
	typename... Args
>
auto makeSetter(
		SampledFunctionCollectionImpl* obj,
		Args... args
);

template <auto function>
void run(
		SetterTask<function>* setter
);
