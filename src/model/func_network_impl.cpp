#include "fge/model/func_network_impl.h"
#include "include/fge/model/func_network.h"


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
	);
}

/*********************
 * FunctionCollectionImpl
*********************/

FunctionCollectionImpl::FunctionCollectionImpl()
	: constants( symbols() )
{}

uint FunctionCollectionImpl::size() const
{
	return entries.size();
}

FunctionOrError FunctionCollectionImpl::get(const Index index) const
{
	auto entry = entries.at( index );
	return entry->functionOrError
	.transform_error([](auto invalidEntry) {
			return invalidEntry.error;
	});
}

FunctionParameters FunctionCollectionImpl::getFunctionParameters(const uint index) const
{
	auto entry = entries.at( index );
	if( entry->functionOrError ) {
		auto function = entry->functionOrError.value();
		return FunctionParameters{
			.formula = function->toString(),
			.parameters = function->getParameters(),
			.stateDescriptions = function->getStateDescriptions()
		};
	}
	else {
		auto error = entry->functionOrError.error();
		return error.parameters;
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
						.parameters = FunctionParameters{
								.formula = (i>0)
									? QString("%1(x)").arg( functionName(i-1) )
									: "cos( 2pi * x )"
						}
					}),
					.info = createNodeInfo()
			});
			entries.push_back( entry );
		}
		updateFormulas(oldSize, {});
	}
	assert( entries.size() == size );
}

MaybeError FunctionCollectionImpl::set(
		const Index index,
		const FunctionParameters& parameters
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
					entry->functionOrError.value().get()
			);
		}
	}
	updateFormulas( index, parameters );
	if( !get(index) ) {
		return get(index).error();
	}
	return {};
}

MaybeError FunctionCollectionImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	FunctionOrError functionOrError = get( index );
	if( !functionOrError ) {
		return functionOrError.error();
	}
	auto function = functionOrError.value();
	for( auto [key, val] : parameters ) {
		auto maybeError = function->setParameter( key, val );
		if( maybeError ) {
			return maybeError.value();
		}
	}
	return {};
}

void FunctionCollectionImpl::updateFormulas(
		const size_t startIndex,
		const std::optional<FunctionParameters>& parameters
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
					entry->functionOrError.value().get()
			);
		}
	}
	/* update entries
	 * from startIndex:
	 */
	for( size_t i=startIndex; i<entries.size(); i++ ) {
		auto entry = entries.at(i);
		FunctionParameters params = getFunctionParameters(i);
		if( i==startIndex && parameters ) {
			params = parameters.value();
		}
		entry->functionOrError = formulaFunctionFactory(
				params.formula,
				params.parameters,
				params.stateDescriptions,
				{
					constants,
					functionSymbols
				}
		).transform_error([params](auto error){
			return InvalidEntry{
				.error = error,
				.parameters = params
			};
		});
		if( entry->functionOrError ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
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
