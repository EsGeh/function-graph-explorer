#include "fge/model/model.h"
#include "fge/shared/utils.h"
#include "include/fge/model/function.h"
#include <cstdio>
#include <functional>
#include <mutex>


inline QString functionName( const size_t index ) {
	return QString("f%1").arg( index );
}


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

DECL_FUNC_BEGIN(RandomFunction,0,)
	return C(T(std::rand())/T(RAND_MAX), T(std::rand())/T(RAND_MAX));
DECL_FUNC_END(RandomFunction)

DECL_FUNC_BEGIN(Real_Compare,2,const C& x1, const C& x2)
	T ret = 0;
	if( x1.c_.real() > x2.c_.real() )
		ret = 1;
	else if( x1.c_.real() < x2.c_.real() )
		ret = -1;
	return C(ret, 0);
DECL_FUNC_END(Real_Compare)

static auto realFunc = RealFunction();
static auto imagFunc = ImagFunction();

static auto absFunc = AbsFunction();
static auto argFunc = ArgFunction();
static auto conjFunc = ConjFunction();
static auto complexFunc = ComplexFunction();
static auto polarFunc = PolarFunction();
static auto realCompare = Real_Compare();
static auto randomFunc = RandomFunction();

Model::Model(
		const SamplingSettings& defSamplingSettings
)
	: constants(
		{
			{ "pi", C(acos(-1),0) },
			{ "e", cmplx::details::constant::e },
			{ "i", C(0,1) },
		},

		// functions:
		{
			{ "abs", &absFunc },
			{ "real", &realFunc },
			{ "imag", &imagFunc },
			{ "arg", &argFunc },
			{ "conj", &conjFunc },
			{ "complex", &complexFunc },
			{ "polar", &polarFunc },
			{ "real_cmp", &realCompare },
			{ "rnd", &randomFunc }
		}
	)
	, defSamplingSettings( defSamplingSettings )
{}

#define WITH_LOCK() \
	const std::scoped_lock<std::mutex> guard(lock);

size_t Model::size()
{
	WITH_LOCK()
	return functions.size();
}

QString Model::getFormula(
		const size_t index
)
{
	WITH_LOCK()
	return functions.at( index )->formula;
}

MaybeError Model::getError(
		const size_t index
)
{
	WITH_LOCK()
	return getErrorPrivate(index);
}

SamplingSettings Model::getSamplingSettings(
		const size_t index
)
{
	WITH_LOCK()
	return functions.at( index )->samplingSettings;
}

void Model::setSamplingSettings(
		const size_t index,
		const SamplingSettings& value
)
{
	WITH_LOCK()
	functions.at( index )->samplingSettings = value;
	auto errorOrFunction = getFunctionPrivate( index );
	if( errorOrFunction ) {
		auto function = errorOrFunction.value();
		function->setResolution( value.resolution );
		function->setInterpolation( value.interpolation );
		function->setCaching( value.caching );
	}
}

ErrorOrValue<ParameterBindings> Model::getParameters(
		const size_t index
)
{
	WITH_LOCK()
	ErrorOrFunction errorOrFunction = functions.at( index )->errorOrFunction;
	if( !errorOrFunction ) {
		return std::unexpected(errorOrFunction.error());
	}
	auto function = errorOrFunction.value();
	return function->getParameters();
}

MaybeError Model::setParameterValues(
		const size_t index,
		const ParameterBindings& parameters
)
{
	WITH_LOCK()
	ErrorOrFunction errorOrFunction = functions.at( index )->errorOrFunction;
	if( !errorOrFunction ) {
		return errorOrFunction.error();
	}
	auto function = errorOrFunction.value();
	for( auto [key, val] : parameters ) {
		auto maybeError = function->setParameter( key, val.at(0) );
		if( maybeError ) {
			return maybeError.value();
		}
	}
	return {};
}

ErrorOrValue<StateDescriptions> Model::getStateDescriptions(
		const size_t index
)
{
	WITH_LOCK()
	ErrorOrFunction errorOrFunction = functions.at( index )->errorOrFunction;
	if( !errorOrFunction ) {
		return std::unexpected(errorOrFunction.error());
	}
	auto function = errorOrFunction.value();
	return function->getStateDescriptions();
}

ErrorOrValue<std::vector<std::pair<C,C>>> Model::getGraph(
		const size_t index,
		const std::pair<T,T>& range,
		const unsigned int resolution
)
{
	WITH_LOCK()
	auto entry = functions.at( index );
	auto errorOrFunction = entry->errorOrFunction;
	if( !errorOrFunction ) {
		return std::unexpected( errorOrFunction.error() );
	}
	{
		auto function = errorOrFunction.value();
		auto
			xMin = range.first,
			xMax = range.second
		;
		std::vector<std::pair<C,C>> graph;
		for( unsigned int i=0; i<resolution; i++ ) {
			auto x = C( xMin + (T(i) / (resolution-1))*(xMax - xMin), 0);
			graph.push_back(
					{
						x,
						function->get(x)
					}
			);
		}
		return graph;
	}
}

float Model::audioFunction(
		const unsigned long int position,
		const uint samplerate
)
{
	WITH_LOCK()
	double ret = 0;
	C time = C(T(position) / T(samplerate), 0);
	for( auto entry : functions ) {
		if( !(entry->errorOrFunction) || !(entry->isAudioEnabled) )
			continue;
		auto function = entry->errorOrFunction.value();
		ret += function->get( time ).c_.real();
	}
	return std::clamp( ret, -1.0, +1.0 );
}

bool Model::getIsPlaybackEnabled(
		const uint index
)
{
	WITH_LOCK()
	auto entry = functions.at( index );
	return entry->isAudioEnabled;
}

void Model::setIsPlaybackEnabled(
		const uint index,
		const bool value
)
{
	WITH_LOCK()
	auto entry = functions.at( index );
	entry->isAudioEnabled = value;
}

void Model::resize( const size_t size ) {
	WITH_LOCK()
	const auto oldSize = functions.size();
	if( size < oldSize ) {
		functions.resize( size );
	}
	else if( size > oldSize ) {
		for( auto i=oldSize; i<size; i++ ) {
			auto entry = std::shared_ptr<FunctionEntry>(new FunctionEntry {
				.formula = (i>0)
					? QString("%1(x)").arg( functionName(i-1) )
					: "cos( 2pi * x )"
			});
			entry->samplingSettings = defSamplingSettings;
			functions.push_back( entry );
		}
		updateFormulas(oldSize, {}, {});
	}
	assert( functions.size() == size );
}

MaybeError Model::set(
		const size_t index,
		const QString& formula,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescriptions
) {
	WITH_LOCK()
	// assert( index < size() );
	auto entry = functions[index];
	entry->formula = formula;
	updateFormulas(
			index,
			parameters,
			stateDescriptions
	);
	return getErrorPrivate( index );
}

ErrorOrFunction Model::getFunction(const size_t index)
{
	WITH_LOCK()
	return getFunctionPrivate(index);
}

MaybeError Model::getErrorPrivate(
		const size_t index
) const
{
	auto errorOrFunction = getFunctionPrivate(index);
	if( !errorOrFunction ) {
		return errorOrFunction.error();
	}
	return {};
}

ErrorOrFunction Model::getFunctionPrivate(const size_t index) const
{
	return functions.at( index )->errorOrFunction;
}

void Model::updateFormulas(
		const size_t startIndex,
		const std::optional<ParameterBindings>& setBindings,
		const std::optional<StateDescriptions>& setStateDescriptions
)
{
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = functions.at(i);
		if( entry->errorOrFunction.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->errorOrFunction.value().get()
			);
		}
	}
	/* update entries
	 * from startIndex:
	 */
	for( size_t i=startIndex; i<functions.size(); i++ ) {
		auto entry = functions.at(i);
		ParameterBindings parameters = {};
		entry->errorOrFunction.transform([&parameters](auto function) {
			parameters = function->getParameters();
		});
		if( i==startIndex && setBindings ) {
			parameters = setBindings.value();
		}
		StateDescriptions stateDescriptions = {};
		entry->errorOrFunction.transform([&stateDescriptions](auto function) {
			stateDescriptions = function->getStateDescriptions();
		});
		if( i==startIndex && setStateDescriptions ) {
			stateDescriptions = setStateDescriptions.value();
		}
		entry->errorOrFunction = formulaFunctionFactory(
				entry->formula,
				parameters,
				stateDescriptions,
				{
					constants,
					functionSymbols
				},
				entry->samplingSettings.resolution,
				entry->samplingSettings.interpolation,
				entry->samplingSettings.caching
		);
		if( entry->errorOrFunction ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->errorOrFunction.value().get()
			);
		}
	}
}
