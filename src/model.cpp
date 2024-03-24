#include "model.h"


const unsigned int RES = 1024;


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

FormulaFunction::FormulaFunction()
	: Function(),
	formulaStr(""),
	sym_table(symbol_table_t::symtab_mutability_type::e_immutable)
{
	sym_table.add_constant("pi", acos(-1));
	sym_table.add_variable("x", varX);
	formula.register_symbol_table( sym_table );
}

T FormulaFunction::get( T x ) {
	varX = x;
	return formula.value();
}

QString FormulaFunction::toString() const {
	return formulaStr;
}

bool FormulaFunction::init(const QString& formulaStr) {
	this->formulaStr = formulaStr;
	parser_t parser;
	parser.settings().disable_all_control_structures();
	parser.settings().disable_all_assignment_ops();
	return parser.compile( formulaStr.toStdString(), formula);
}

std::optional<std::shared_ptr<Function>> formulaFunctionFactory(
	const QString& formulaStr
) {
	auto ret = std::shared_ptr<FormulaFunction>(new FormulaFunction());
	if( ! ret->init( formulaStr ) ) {
		return {};
	}
	return ret;
}
