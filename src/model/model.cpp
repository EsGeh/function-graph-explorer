#include "fge/model/model.h"
#include "fge/shared/utils.h"
#include "include/fge/model/function.h"


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

static auto realFunc = RealFunction();
static auto imagFunc = ImagFunction();

static auto absFunc = AbsFunction();
static auto argFunc = ArgFunction();
static auto conjFunc = ConjFunction();
static auto complexFunc = ComplexFunction();
static auto polarFunc = PolarFunction();
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
			{ "rnd", &randomFunc }
		}
	)
	, functions()
	, defSamplingSettings( defSamplingSettings )
{}

size_t Model::size() const
{
	return functions.size();
}

QString Model::getFormula(
		const size_t index
) const 
{
	return functions.at( index )->string;
}

MaybeError Model::getError(
		const size_t index
) const
{
	auto errorOrFunction = getFunction(index);
	if( !errorOrFunction ) {
		return errorOrFunction.error();
	}
	return {};
}
SamplingSettings Model::getSamplingSettings(
		const size_t index
) const
{
	return functions.at( index )->samplingSettings;
}

void Model::setSamplingSettings(
		const size_t index,
		const SamplingSettings& value
)
{
	functions.at( index )->samplingSettings = value;
	auto errorOrFunction = getFunction( index );
	if( errorOrFunction ) {
		auto function = errorOrFunction.value();
		function->setResolution( value.resolution );
		function->setInterpolation( value.interpolation );
		function->setCaching( value.caching );
	}
}

ParameterBindings Model::getParameters(
		const size_t index
) const
{
	return functions.at( index ) ->parameters;
}

void Model::setParameters(
		const size_t index,
		const ParameterBindings& parameters
)
{
	functions.at( index )->parameters = parameters;
}

ErrorOrValue<std::vector<std::pair<C,C>>> Model::getGraph(
		const size_t index,
		const std::pair<T,T>& range,
		const unsigned int resolution
) const
{
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
						function->get(
								x,
								entry->parameters
						),
					}
			);
		}
		return graph;
	}
}

MaybeError Model::valuesToAudioBuffer(
		const size_t index,
		std::vector<float>* buffer,
		const T duration,
		const T speed,
		const T offset,
		const unsigned int samplerate,
		std::function<float(const double)> volumeFunction
) const
{
	auto entry = functions.at( index );
	auto errorOrFunction = entry->errorOrFunction;
	if( !errorOrFunction ) {
		return errorOrFunction.error();
	}
	{
		auto function = errorOrFunction.value();
		const unsigned int countSamples = duration*samplerate; 
		buffer->resize( countSamples );
		for( unsigned int i=0; i<countSamples; i++ ) {
			T time = T(i)/samplerate;
			T y = function->get(
					C(speed * time + offset, 0),
					entry->parameters
			);
			T vol = volumeFunction( time );
			y = y * vol;
			y = std::clamp( y, -1.0, +1.0 );
			buffer->at(i) = y;
		}
	}
	return {};
}

void Model::resize( const size_t size ) {
	const auto oldSize = functions.size();
	if( size < oldSize ) {
		functions.resize( size );
	}
	else if( size > oldSize ) {
		for( auto i=oldSize; i<size; i++ ) {
			auto entry = std::shared_ptr<FunctionEntry>(new FunctionEntry {
				(i>0)
					? QString("%1(x)").arg( functionName(i-1) )
					: "cos( 2pi * x )",
				{}
			});
			entry->samplingSettings = defSamplingSettings;
			functions.push_back( entry );
		}
		updateFormulas(oldSize);
	}
	assert( this->size() == size );
}

MaybeError Model::set( const size_t index, const QString& functionStr ) {
	// assert( index < size() );
	auto entry = functions[index];
	entry->string = functionStr;
	updateFormulas( index );
	return getError( index );
}

ErrorOrFunction Model::getFunction(const size_t index) const
{
	return functions.at( index )->errorOrFunction;
}

void Model::updateFormulas(const size_t startIndex) {

	// symbol_table_t functionSymbols(symbol_table_t::symtab_mutability_type::e_immutable);
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = functions.at(i);
		if( entry->errorOrFunction ) {
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
		entry->errorOrFunction = formulaFunctionFactory(
				entry->string,
				descrFromParameters( entry->parameters ),
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
