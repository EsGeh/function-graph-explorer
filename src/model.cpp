#include "model.h"
#include <QDebug>


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

static auto realFunc = RealFunction();
static auto imagFunc = ImagFunction();

static auto absFunc = AbsFunction();
static auto argFunc = ArgFunction();
static auto conjFunc = ConjFunction();
static auto complexFunc = ComplexFunction();
static auto polarFunc = PolarFunction();

Model::Model()
	: constantSymbols(symbol_table_t::symtab_mutability_type::e_immutable)
	, functionSymbols(symbol_table_t::symtab_mutability_type::e_immutable)
	, functions()
{
	constantSymbols.add_constant( "pi", C(acos(-1),0) );
	constantSymbols.add_constant( "e", cmplx::details::constant::e );
	constantSymbols.add_constant( "i", C(0,1) );

	// complex functions:
	constantSymbols.add_function( "abs", absFunc );
	constantSymbols.add_function( "real", realFunc );
	constantSymbols.add_function( "imag", imagFunc );
	constantSymbols.add_function( "arg", argFunc );
	constantSymbols.add_function( "conj", conjFunc );
	constantSymbols.add_function( "complex", complexFunc );
	constantSymbols.add_function( "polar", polarFunc );
}

size_t Model::size() const {
	return functions.size();
}

ErrorOrFunction Model::get(const size_t index) {
	return functions.at( index )->errorOrFunction;
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
			functions.push_back( entry );
		}
		updateFormulas(oldSize);
	}
	assert( this->size() == size );
}

ErrorOrFunction Model::set( const size_t index, const QString& functionStr ) {
	// assert( index < size() );
	auto entry = functions[index];
	entry->string = functionStr;
	updateFormulas( index );
	return entry->errorOrFunction;
}

void Model::updateFormulas(const size_t startIndex) {
	functionSymbols.clear();
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = functions.at(i);
		if( entry->errorOrFunction.index() == 1 ) {
			functionSymbols.add_function(
					functionName( i ).toStdString().c_str(),
					*(std::get<std::shared_ptr<Function>>(
							entry->errorOrFunction
					).get())
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
				{
					&constantSymbols,
					&functionSymbols
				}
		);
		if( entry->errorOrFunction.index() == 1 ) {
			functionSymbols.add_function(
					functionName( i ).toStdString().c_str(),
					*(std::get<std::shared_ptr<Function>>( entry->errorOrFunction ).get())
			);
		}
	}
}
