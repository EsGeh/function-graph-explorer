#include "fge/model/function.h"
#include <optional>


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
	 return this->get( x );
}

/*******************
 * FormulaFunction
 ******************/

FormulaFunction::FormulaFunction()
	: formulaStr("")
{
}

FormulaFunction::~FormulaFunction()
{}

C FormulaFunction::get(
		const C& x
)
{
	varX = x;
	return formula.value();
}

QString FormulaFunction::toString() const {
	return formulaStr;
}

ParameterBindings FormulaFunction::getParameters() const
{
	return parameters;
}

MaybeError FormulaFunction::setParameter(
		const QString& name,
		const std::vector<C>& value
)
{
	auto entry = parameters.find( name );
	if( entry == parameters.end() ) {
		return QString("Parameter not found: '%1'").arg( name );
	}
	entry->second = value;
	return {};
}

StateDescriptions FormulaFunction::getStateDescriptions() const
{
	return stateDescriptions;
}

MaybeError FormulaFunction::init(
		const QString& formulaStr,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescrs,
		const std::vector<Symbols>& additionalSymbols
)
{
	this->formulaStr = formulaStr;
	this->parameters = parameters;
	this->stateDescriptions = stateDescrs;

	for( auto [name, descr] : stateDescrs )
	{
		std::vector<C> vector(descr.size, C(0,0) );
		this->state.insert( { name, vector } );
	}

	// build symbol table
	// add "x" and parameters:
	symbol_table_t symbols;
	symbols.add_variable( "x", varX );
	// parameters:
	for( auto& [paramName, paramValue] : this->parameters ) {
		if( paramValue.size() == 1 ) {
			symbols.add_constant(
					paramName.toStdString(),
					paramValue[0]
			);
		}
		else {
			symbols.add_vector(
					paramName.toStdString(),
					paramValue
			);
		}
	}
	// add state:
	for( auto& [key, value] : this->state ) {
		if( stateDescriptions[key].size == 1 ) {
			symbols.add_variable( key.toStdString(), value.at(0) );
		}
		else {
			symbols.add_vector( key.toStdString(), value );
		}
	}
	// add additional symbols:
	formula.register_symbol_table( symbols );
	for( auto additional : additionalSymbols ) {
		formula.register_symbol_table( additional.get() );
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
		const uint resolution,
		const uint interpolation,
		const bool caching
)
	: FormulaFunction()
	, resolution(resolution)
	, interpolation(interpolation)
	, cache( [this](int x){ return rasterIndexToY(x); } )
{}

C FunctionWithResolution::get(
		const C& x
)
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
		const int xpos = xToRasterIndex(x) + i - shift;
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

MaybeError FunctionWithResolution::setParameter(
		const QString& name,
		const std::vector<C>& value
)
{
	auto maybeError = FormulaFunction::setParameter( name, value );
	if( !maybeError ) {
		cache.clear();
	}
	return maybeError;
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
		int x
)
{
	return FormulaFunction::get(
			C(T(x) / getResolution(),0)
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
		const ParameterBindings& parameters,
		const StateDescriptions& state,
		const std::vector<Symbols>& additionalSymbols,
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
	auto maybeError = function->init(
			formulaStr,
			parameters,
			state,
			additionalSymbols
	);
	if( maybeError.has_value() ) {
		return std::unexpected(maybeError.value() );
	}
	return function;
}

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		const ParameterBindings& parameters,
		const StateDescriptions& state,
		const std::vector<Symbols>& additionalSymbols,
		const uint resolution, // = 0
		const uint interpolation, // = 0
		const bool enableCaching // = true 
) {
	return formulaFunctionFactory_internal(
			formulaStr,
			parameters,
			state,
			additionalSymbols,
			resolution,
			interpolation,
			enableCaching
	);
}
