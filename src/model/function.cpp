#include "model/function.h"
#include <memory>
#include <utility>



Function::Function()
	: exprtk::ifunction<C>(1)
{
}

Function::~Function() 
{}

C Function::operator()(const C& x)
{
	 return this->get( x );
}

FormulaFunction::FormulaFunction()
	: Function()
	, formulaStr("")
{
}

FormulaFunction::~FormulaFunction()
{}

C FormulaFunction::get( C x ) {
	varX = x;
	return formula.value();
}

QString FormulaFunction::toString() const {
	return formulaStr;
}

MaybeError FormulaFunction::init(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols
) {
	this->formulaStr = formulaStr;

	symbol_table_t symbols;
	symbols.add_variable( "x", varX );
	formula.register_symbol_table( symbols );
	for( auto table : additionalSymbols ) {
		formula.register_symbol_table( *table );
	}

	parser_t parser;
	// parser.settings().disable_all_control_structures();
	// parser.settings().disable_all_assignment_ops();
	auto ret = parser.compile( formulaStr.toStdString(), formula);
	if( !ret ) {
      return parser.error().c_str();
	}
	return {};
}

CachedFunction::CachedFunction(
		const uint resolution,
		const bool enableInterpolate
)
	: FormulaFunction()
	, resolution( resolution )
	, enableInterpolate( enableInterpolate )
	, function( [this,resolution](int x){
			return FormulaFunction::get( C(T(x) / resolution,0) );
		} )
	, cache( this->function )
{}

CachedFunction::~CachedFunction()
{}

C CachedFunction::interpolate(
		const C& x,
		const C ys[2]
)
{
	T i;
	C xfrac = C(
			modf( x.c_.real() * resolution, &i),
			modf( x.c_.imag() * resolution, &i)
	);
	return ys[0] + xfrac * (ys[1] - ys[0]);
}

C CachedFunction::get( C x )
{
	if(
			x.c_.imag() != 0
			|| resolution == 0
	) {
		return FormulaFunction::get( x );
	}
	if( !enableInterpolate ) {
		return cache.lookup( floor(x.c_.real() * resolution) ).first;
	}
	C ys[] = {
		cache.lookup( floor(x.c_.real() * resolution) ).first,
		cache.lookup( floor(x.c_.real() * resolution)+1 ).first,
	};
	return interpolate(x, ys);
}

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution,
		const bool enableInterpolate
)
{
	auto function = std::shared_ptr<CachedFunction>(new CachedFunction(
				resolution,
				enableInterpolate
	));
	auto maybeError = function->init( formulaStr, additionalSymbols);
	if( maybeError ) {
		return std::unexpected(maybeError.value() );
	}
	return function;
}

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution, // = 0
		const bool enableInterpolate // = false
) {
	return formulaFunctionFactory_internal(
			formulaStr,
			additionalSymbols,
			resolution,
			enableInterpolate
	);
}
