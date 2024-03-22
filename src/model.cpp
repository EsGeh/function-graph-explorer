#include "model.h"


const unsigned int RES = 100;

Model::Model()
:
	sym_table(symbol_table_t::symtab_mutability_type::e_immutable),
	isValidExpression(false),
	varX(0),
	xMin(-1), xMax(1)
{
	sym_table.add_constant("pi", acos(-1));
	sym_table.add_variable("x", varX);
	formula.register_symbol_table( sym_table );
}

bool Model::getIsValidExpression() const {
	return isValidExpression;
}

std::vector<std::pair<T,T>> Model::getPoints() {
	if( !isValidExpression) {
		return {};
	}
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

T Model::getXMin() {
	return xMin;
}
T Model::getXMax() {
	return xMax;
}

void Model::set(const QString& formula_str) {
	parser_t parser;
	parser.settings().disable_all_control_structures();
	parser.settings().disable_all_assignment_ops();
	if( !parser.compile( formula_str.toStdString(), formula) ) {
		isValidExpression = false;
		return;
	}
	isValidExpression = true;
}

void Model::setXMin(T value) {
	xMin = value;
}
void Model::setXMax(T value) {
	xMax = value;
}

void Model:: resetXMin() {
	xMin = -1;
}
void Model:: resetXMax() {
	xMax = 1;
}

T Model::get( T x ) {
	varX = x;
	return formula.value();
}