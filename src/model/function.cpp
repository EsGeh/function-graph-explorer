#include "model/function.h"
#include <QDebug>



Function::Function()
	: exprtk::ifunction<C>(1)
{
}

C Function::operator()(const C& x)
{
	 return this->get( x );
}

FormulaFunction::FormulaFunction()
	: Function()
	, formulaStr("")
{
}

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

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
	const QString& formulaStr,
	std::vector<symbol_table_t*> additionalSymbols
) {
	auto function = std::shared_ptr<FormulaFunction>(new FormulaFunction());
	auto maybeError = function->init( formulaStr, additionalSymbols);
	if( maybeError ) {
		return maybeError.value();
	}
	return function;
}
