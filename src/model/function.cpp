#include "fge/model/function.h"
#include <optional>
#include <format>
#include <stdexcept>


Symbols::Symbols(
		const std::map<QString,C>& constants,
		const std::map<QString,exprtk::ifunction<C>*>& functions
)
	: symbols(symbol_table_t::symtab_mutability_type::e_immutable)
{
	for( auto [key, val] : constants ) {
		addConstant( key, val );
	}
	for( auto [key, val] : functions ) {
		addFunction( key, val );
	}
}

void Symbols::addConstant(
		const QString& name,
		const C& value
)
{
	symbols.add_constant( name.toStdString(), value );
}

void Symbols::addFunction(
		const QString& name,
		exprtk::ifunction<C>* function
)
{
	symbols.add_function( name.toStdString(), *function );
}

symbol_table_t& Symbols::get()
{
	return symbols;
}

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
	 return this->get( x, {} );
}

/*******************
 * FormulaFunction
 ******************/

FormulaFunction::FormulaFunction(
		const std::vector<QString>& parameters // = {}
)
	: formulaStr("")
{
	for( auto p : parameters )
	{
		this->parameters.insert( { p, C(0,0) } );
	}
}

FormulaFunction::~FormulaFunction()
{}

C FormulaFunction::get(
		const C& x,
		const ParameterBindings& parameters
)
{
	for( auto& [key, value] : this->parameters ) {
		auto el = parameters.find( key );
		if( el == parameters.end() ) {
			throw std::invalid_argument(
					std::format("Parameter not found: {}", key.toStdString())
			);
		}
		value = el->second;
	}
	varX = x;
	return formula.value();
}

QString FormulaFunction::toString() const {
	return formulaStr;
}

ParameterDescription FormulaFunction::getParameterDescription() const
{
	decltype(getParameterDescription()) ret;
	for( auto [key, _] : parameters ) {
		ret.push_back( key );
	}
	return ret;
}

MaybeError FormulaFunction::init(
		const QString& formulaStr,
		const std::vector<Symbols>& additionalSymbols
		// std::vector<symbol_table_t*> additionalSymbols
) {
	this->formulaStr = formulaStr;

	// build symbol table
	// add "x" and parameters:
	symbol_table_t symbols;
	symbols.add_variable( "x", varX );
	for( auto& [paramName, paramValue] : parameters ) {
		symbols.add_variable( paramName.toStdString(), paramValue );
	}
	// add additional symbols:
	formula.register_symbol_table( symbols );
	for( auto symbols : additionalSymbols ) {
		formula.register_symbol_table( symbols.get() );
	}

	// parse formula:
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
		const std::vector<QString>& parameters,
		const uint resolution,
		const uint interpolation,
		const bool caching
)
	: FormulaFunction( parameters )
	, resolution(resolution)
	, interpolation(interpolation)
	, cache( [this](int x, const ParameterBindings& parameters){ return rasterIndexToY(x, parameters); } )
{}

C FunctionWithResolution::get(
		const C& x,
		const ParameterBindings& parameters
)
{
	if(
			resolution == 0
			|| x.c_.imag() != 0
	) {
		return FormulaFunction::get(x, parameters);
	}
	static std::optional<ParameterBindings> lastParameters = {};
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
	// clear cache, if a parameter changed:
	if( caching ) {
		bool changed = false;
		if( parameters.size() > 0 && lastParameters.has_value() ) {
			for(auto [key,val]: parameters) {
				if( lastParameters.value()[key] != val ) {
					changed = true;
					break;
				}
			}
		}
		if( changed ) {
			cache.clear();
		}
		lastParameters = parameters;
	}
	for( int i{0}; i<int(interpolation+1); i++ ) {
		const int xpos = xToRasterIndex(x) + i - shift;
		if( caching ) {
			ys.push_back(cache.lookup( xpos, parameters ).first);
		}
		else {
			ys.push_back(FormulaFunction::get(
					C(
						T(xpos) / getResolution(),
						0
					),
					parameters
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

FunctionWithResolution::RasterIndex FunctionWithResolution::xToRasterIndex(const C& x)
{
		return std::floor(x.c_.real() * getResolution() + epsilon);
}

C FunctionWithResolution::rasterIndexToY(
		int x,
		const ParameterBindings& parameters
)
{
	return FormulaFunction::get(
			C(T(x) / getResolution(),0),
			parameters
	);
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
		const std::vector<QString>& parameters,
		const uint resolution,
		const uint interpolation,
		const bool enableCaching
)
	: FunctionWithResolution(
			parameters,
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
		const std::vector<QString>& parameters,
		const std::vector<Symbols>& additionalSymbols,
		const uint resolution,
		const uint interpolation,
		const bool enableCaching
)
{
	auto function = std::shared_ptr<FunctionImpl>(new FunctionImpl(
				parameters,
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
		const std::vector<QString>& parameters,
		const std::vector<Symbols>& additionalSymbols,
		const uint resolution, // = 0
		const uint interpolation, // = 0
		const bool enableCaching // = true 
) {
	return formulaFunctionFactory_internal(
			formulaStr,
			parameters,
			additionalSymbols,
			resolution,
			interpolation,
			enableCaching
	);
}
