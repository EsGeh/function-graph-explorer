#include "fge/model/function.h"
#include <memory>
#include <optional>
#include <QDebug>


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
		const C& value
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
		symbols.add_variable(
				paramName.toStdString(),
				paramValue
		);
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
 * Fabric method:
 ******************/
ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		const ParameterBindings& parameters,
		const StateDescriptions& state,
		const std::vector<Symbols>& additionalSymbols
)
{
	auto function = std::shared_ptr<FormulaFunction>(new FormulaFunction());
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
