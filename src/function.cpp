#include "function.h"
#include <climits>
#include <optional>
#include <QDebug>


const unsigned int RES = 1024;


Function::Function()
	: exprtk::ifunction<T>(1)
{
}

std::vector<std::pair<T,T>> Function::getPoints(
		const std::pair<T,T>& range
) {
	auto
		xMin = range.first,
		xMax = range.second
	;
	std::vector<std::pair<T,T>> graph;
	for( unsigned int i=0; i<RES+1; i++ ) {
		T x = xMin + (T(i) / RES)*(xMax - xMin);
		graph.push_back(
				{
					x,
					get( x ),
				}
		);
	}
	return graph;
}

T Function::operator()(const T& x)
{
	 return this->get( x );
}

FormulaFunction::FormulaFunction()
	: Function()
	, formulaStr("")
{
}

T FormulaFunction::get( T x ) {
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
