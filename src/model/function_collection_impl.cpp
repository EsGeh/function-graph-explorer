#include "fge/model/function_collection_impl.h"
#include "include/fge/model/function.h"
#include "include/fge/model/function_collection.h"
#include <exprtk.hpp>
#include <qlogging.h>
#include <QDebug>


#define DECL_FUNC_BEGIN(CLASS, ORD, ...) \
struct CLASS: \
	public exprtk::ifunction<C>  \
{ \
	CLASS() \
	: exprtk::ifunction<C>(ORD) \
	{} \
\
	C operator()( __VA_ARGS__ ) {

#define DECL_FUNC_END(CLASS) \
	} \
};

DECL_FUNC_BEGIN(RealFunction,1,const C& x)
	return C(std::real( x.c_ ));
DECL_FUNC_END(RealFunction)

DECL_FUNC_BEGIN(ImagFunction,1,const C& x)
	return C(std::imag( x.c_ ));
DECL_FUNC_END(ImagFunction)

DECL_FUNC_BEGIN(AbsFunction,1,const C& x)
	return C(std::abs( x.c_ ));
DECL_FUNC_END(AbsFunction)

DECL_FUNC_BEGIN(ArgFunction,1,const C& x)
	return C(std::arg( x.c_ ));
DECL_FUNC_END(ArgFunction)

DECL_FUNC_BEGIN(ConjFunction,1,const C& x)
	return C(std::conj( x.c_ ));
DECL_FUNC_END(ConjFunction)

DECL_FUNC_BEGIN(ComplexFunction,2,const C& x1, const C& x2)
	return C(T(x1), T(x2));
DECL_FUNC_END(ComplexFunction)

DECL_FUNC_BEGIN(PolarFunction,2,const C& x1, const C& x2)
	return C(std::polar( T(x1), T(x2) ));
DECL_FUNC_END(PolarFunction)

DECL_FUNC_BEGIN(ComplexMod,2,const C& x1, const C& x2)
	return C(fmod(x1.c_.real(), x2.c_.real()), fmod(x1.c_.imag(),x2.c_.imag()));
DECL_FUNC_END(ComplexMod)

DECL_FUNC_BEGIN(RandomFunction,0,)
	return C(T(std::rand())/T(RAND_MAX), 0);
DECL_FUNC_END(RandomFunction)

DECL_FUNC_BEGIN(MidiToFreq,1,const C& x)
	return C( 440.0 * powf(2,1.0/12.0 * (x.c_.real()-69.0)) ,0);
DECL_FUNC_END(MidiToFreq)

DECL_FUNC_BEGIN(Real_Compare,2,const C& x1, const C& x2)
	T ret = 0;
	if( x1.c_.real() > x2.c_.real() )
		ret = 1;
	else if( x1.c_.real() < x2.c_.real() )
		ret = -1;
	return C(ret, 0);
DECL_FUNC_END(Real_Compare)

uint bitInvImpl( const uint size, const uint value ) {
	uint ret = 0;
	uint valueCopy = value;
	{
		uint shift = sizeof(uint)*8 - size;
		valueCopy <<= shift;
		valueCopy >>= shift;
		for( uint i=0; i<size; i++ ) {
			ret <<= 1;
			ret |= (valueCopy & uint(1));
			valueCopy >>= 1;
		}
	}
	return ret;
}

DECL_FUNC_BEGIN(BitInversion,2,const C& x1, const C& x2)
	return C(
		bitInvImpl( uint(x1.c_.real()), floor(x2.c_.real()) )
	, 0);
DECL_FUNC_END(BitInversion)

struct BitRevCopy:
	public exprtk::igeneric_function<C>
{
	using parameter_list_t = exprtk::igeneric_function<C>::parameter_list_t;

	BitRevCopy()
	: exprtk::igeneric_function<C>("VV")
	{}

	C operator()(parameter_list_t parameters) {
		using generic_type = exprtk::igeneric_function<C>::generic_type;
		using vector_t = generic_type::vector_view;
		vector_t out(parameters[0]);
		vector_t in(parameters[1]);
		for( uint i=0; i<std::min(in.size(),out.size()); i++ ) {
			out[i] = in[ bitInvImpl( ceil(log2(out.size())), i) ];
		}
		return C(0,0);
	}
};

struct Print:
	public exprtk::igeneric_function<C>
{
	using parameter_list_t = exprtk::igeneric_function<C>::parameter_list_t;

	Print()
	: exprtk::igeneric_function<C>()
	{}

	C operator()(parameter_list_t parameters) {
 		typedef typename generic_type::scalar_view scalar_t;
		// typedef typename generic_type::vector_view vector_t;
		typedef typename generic_type::string_view string_t;
		
		std::stringstream strstr;
		for (std::size_t i = 0; i < parameters.size(); ++i)
		{
			generic_type& gt = parameters[i];

			if (generic_type::e_scalar == gt.type)
			{
				scalar_t x(gt);
				strstr << to_qstring( x.v_ ).toStdString();
			}
			/*
			else if (generic_type::e_vector == gt.type)
			{
				vector_t vector(gt);
			}
			*/
			else if (generic_type::e_string == gt.type)
			{
				string_t string(gt);
				strstr << (char* )string.data_;
			}
		}
		qDebug() << strstr.str();
		return C(0,0);
	}
};

static auto print_func = Print();
static auto realFunc = RealFunction();
static auto imagFunc = ImagFunction();

static auto absFunc = AbsFunction();
static auto argFunc = ArgFunction();
static auto conjFunc = ConjFunction();
static auto complexFunc = ComplexFunction();
static auto polarFunc = PolarFunction();
static auto realCompare = Real_Compare();
static auto complexMod = ComplexMod();
static auto randomFunc = RandomFunction();
static auto mtof = MidiToFreq();
static auto bitinv = BitInversion();
static auto bit_rev_copy = BitRevCopy();

Symbols symbols()
{
	return Symbols(
		{
			{ "pi", C(acos(-1),0) },
			{ "e", cmplx::details::constant::e },
			{ "i", C(0,1) },
		},

		// functions:
		{
			{ "print", &print_func },
			{ "abs", &absFunc },
			{ "real", &realFunc },
			{ "imag", &imagFunc },
			{ "arg", &argFunc },
			{ "conj", &conjFunc },
			{ "complex", &complexFunc },
			{ "polar", &polarFunc },
			{ "real_cmp", &realCompare },
			{ "c_mod", &complexMod },
			{ "rnd", &randomFunc },
			{ "mtof", &mtof },
			{ "bitinv", &bitinv },
			{ "bitrevcpy", &bit_rev_copy }
		}
	);
}

/*********************
 * FunctionCollectionImpl
*********************/

FunctionCollectionImpl::FunctionCollectionImpl(
		const SamplingSettings& defSamplingSettings
)
	: constants( symbols() )
	, defSamplingSettings( defSamplingSettings )
{}

uint FunctionCollectionImpl::size() const
{
	return entries.size();
}

FunctionOrError FunctionCollectionImpl::getFunction(const Index index) const
{
	auto entry = entries.at( index );
	return entry->functionOrError
		.transform([](auto validEntry) {
				return validEntry.function;
		})
		.transform_error([](auto invalidEntry) {
				return invalidEntry.error;
		});
}

FunctionInfo FunctionCollectionImpl::getFunctionInfo(const uint index) const
{
	auto entry = entries.at( index );
	if( entry->functionOrError ) {
		auto function = entry->functionOrError.value().function;
		auto parameterDescriptions = entry->functionOrError.value().parameterDescriptions;
		return FunctionInfo{
			.formula = function->toString(),
			.parameters = function->getParameters(),
			.parameterDescriptions = parameterDescriptions,
			.stateDescriptions = function->getStateDescriptions()
		};
	}
	else {
		auto error = entry->functionOrError.error();
		return error.functionInfo;
	}
}

void FunctionCollectionImpl::resize( const uint size ) {
	const auto oldSize = entries.size();
	if( size < oldSize ) {
		entries.resize( size );
	}
	else if( size > oldSize ) {
		for( uint i=oldSize; i<size; i++ ) {
			auto entry = std::shared_ptr<NetworkEntry>(new NetworkEntry {
					.functionOrError = std::unexpected(InvalidEntry{
						.error = "not yet initialized",
						.functionInfo = FunctionInfo{
								.formula = (i>0)
									? QString("%1(x)").arg( functionName(i-1) )
									: "cos( 2pi * x )"
						},
						.samplingSettings = defSamplingSettings
					})
			});
			entries.push_back( entry );
		}
		updateFormulas(oldSize, {});
	}
	assert( entries.size() == size );
}

MaybeError FunctionCollectionImpl::set(
		const Index index,
		const FunctionInfo& parameters
) {
	// assert( index < size() );
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<index; i++ ) {
		auto entry = entries.at(i);
		if( entry->functionOrError.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().function.get()
			);
		}
	}
	updateFormulas( index, parameters );
	if( !getFunction(index) ) {
		return getFunction(index).error();
	}
	return {};
}

MaybeError FunctionCollectionImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	FunctionOrError functionOrError = getFunction( index );
	if( !functionOrError ) {
		return functionOrError.error();
	}
	{
		auto function = functionOrError.value();
		for( auto [key, val] : parameters ) {
			auto maybeError = function->setParameter( key, val );
			if( maybeError ) {
				return maybeError.value();
			}
		}
	}
	for( uint i = index; i<size(); i++ ) {
		getFunction(i).transform([](auto function) {
			function->resetState();
		});
	}
	return {};
}

void FunctionCollectionImpl::updateFormulas(
		const size_t startIndex,
		const std::optional<FunctionInfo>& functionInfo
)
{
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = entries.at(i);
		if( entry->functionOrError.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().function.get()
			);
		}
	}
	/* update entries
	 * from startIndex:
	 */
	for( size_t i=startIndex; i<entries.size(); i++ ) {
		auto entry = entries.at(i);
		FunctionInfo currentFunctionInfo = getFunctionInfo(i);
		SamplingSettings samplingSettings = getSamplingSettings(i);
		if( i==startIndex && functionInfo ) {
			currentFunctionInfo = functionInfo.value();
		}
		entry->functionOrError = formulaFunctionFactory(
				currentFunctionInfo.formula,
				currentFunctionInfo.parameters,
				currentFunctionInfo.stateDescriptions,
				{
					constants,
					functionSymbols
				},
				samplingSettings
		)
			.transform([&currentFunctionInfo](auto function) -> ValidEntry {
					return ValidEntry{
						.function = function,
						.parameterDescriptions = currentFunctionInfo.parameterDescriptions
					};
			})
			.transform_error([&currentFunctionInfo, &samplingSettings](auto error) -> InvalidEntry {
				return InvalidEntry{
					.error = error,
					.functionInfo = currentFunctionInfo,
					.samplingSettings = samplingSettings
				};
			});
		if( !entry->info )
		{
			std::shared_ptr<Function> maybeFunction = {};
			if( entry->functionOrError ) {
				maybeFunction = entry->functionOrError.value().function;
			}
			entry->info = createNodeInfo(
					i,
					maybeFunction
			);
		}
		if( entry->functionOrError ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().function.get()
			);
		}
	}
}

FunctionCollectionImpl::NodeInfo* FunctionCollectionImpl::getNodeInfo(
		const Index index
) const
{
	return entries.at(index)->info.get();
}

// Sampling Settings:
SamplingSettings FunctionCollectionImpl::getSamplingSettings(
		const Index index
) const
{
	if( auto function = entries.at(index)->functionOrError; function.has_value() ) {
		return function.value().function->getSamplingSettings();
	}
	else {
		return entries.at(index)->functionOrError.error().samplingSettings;
	}
}

void FunctionCollectionImpl::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{
	if( auto function = entries.at(index)->functionOrError; function.has_value() ) {
			function.value().function->setSamplingSettings( value );
	}
	else {
		auto invalid = entries.at(index)->functionOrError.error();
		invalid.samplingSettings = value;
	}
}
