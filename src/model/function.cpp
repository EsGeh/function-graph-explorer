#include "fge/model/function.h"


/*******************
 * Function
 ******************/

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

/*******************
 * FormulaFunction
 ******************/

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
	auto ret = parser.compile( formulaStr.toStdString(), formula);
	if( !ret ) {
      return parser.error().c_str();
	}
	return {};
}

/*******************
 * FunctionWithResolution
 ******************/

FunctionWithResolution::FunctionWithResolution(
		const uint resolution,
		const uint interpolation,
		const bool caching
)
	: resolution(resolution)
	, interpolation(interpolation)
	, cache( [this](int x){ return cacheIndexToY(x); } )
{}

C FunctionWithResolution::get( C x )
{
	if(
			resolution == 0
			|| x.c_.imag() != 0
	) {
		return FormulaFunction::get(x);
	}
	/* consider `interpolation+1` points,
	 * 	centered around `x`
	 *   x(0-shift) ... x(k+1-shift)
	 *
	 * eg. with interpolation == 3:
	 *
	 *   shift == -1.
	 *   x(-1), x(0), x(1), x(2)
	 * where
	 * 		x is between x(0) and x(1)
	 * 	
	 * */
	const int shift =
		(interpolation != 0)
		? (interpolation+1-2)/2
		: 0;
	std::vector<C> ys;
	for( int i{0}; i<int(interpolation+1); i++ ) {
		const int xpos = xToCacheIndex(x) + i - shift;
		if( caching ) {
			ys.push_back(cache.lookup( xpos ).first);
		}
		else {
			ys.push_back(FormulaFunction::get(
					C(
						T(xpos) / getResolution(),
						0
					)
			));
		}
	};
	return interpolate( x, ys, shift );
}

const T epsilon = 1.0/(1<<20);

C FunctionWithResolution::interpolate(
		const C& x,
		const std::vector<C> ys,
		const int shift

) const
{
	T i_temp;
	T xfrac = std::modf( x.c_.real() * getResolution() + epsilon, &i_temp);
	if( x.c_.real() < 0 ) {
		xfrac *= -1;
		xfrac = 1-xfrac;
	}

	C sum = C(0,0);
	for(int i=0; i<int(ys.size()); i++) {
		T factor = 1;
		for(int j=0; j<int(ys.size()); j++) {
			if( j == i ) { continue; }
			factor *= (xfrac - T(j-shift) );
			factor /= T(i - j);
		}
		sum += ys[i] * C(factor, 0);
	}
	return sum;
}

FunctionWithResolution::CacheIndex FunctionWithResolution::xToCacheIndex(const C& x)
{
		return std::floor(x.c_.real() * getResolution() + epsilon);
}

C FunctionWithResolution::cacheIndexToY(int x)
{
	return FormulaFunction::get( C(T(x) / getResolution(),0) );
}

uint FunctionWithResolution::getResolution() const
{
	return resolution;
}

void FunctionWithResolution::setResolution(const uint value)
{
	resolution = value;
	cache.clear();
}

uint FunctionWithResolution::getInterpolation() const
{
	return interpolation;
}

void FunctionWithResolution::setInterpolation(const uint value)
{
	interpolation = value;
}

bool FunctionWithResolution::getCaching() const
{
	return caching;
}
void FunctionWithResolution::setCaching(const bool value)
{
	caching = value;
}

/*******************
 * FunctionImpl
 ******************/

FunctionImpl::FunctionImpl(
		const uint resolution,
		const uint interpolation,
		const bool enableCaching
)
	: FunctionWithResolution(
			resolution,
			interpolation,
			enableCaching
	)
{
}

/*******************
 * Fabric method:
 ******************/

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution,
		const uint interpolation,
		const bool enableCaching
)
{
	auto function = std::shared_ptr<FunctionImpl>(new FunctionImpl(
				resolution,
				interpolation,
				enableCaching
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
		const uint interpolation, // = 0
		const bool enableCaching // = true 
) {
	return formulaFunctionFactory_internal(
			formulaStr,
			additionalSymbols,
			resolution,
			interpolation,
			enableCaching
	);
}
