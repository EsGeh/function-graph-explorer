#include "model.h"

Model::Model() {
	sym_table.add_variable("x", var_x);
	formula.register_symbol_table( sym_table );
}

void Model::set(const QString& formula_str) {
	parser_t parser;
	if( !parser.compile( formula_str.toStdString(), formula) ) {
		throw "invalid expression";
	}
}

T Model::get( T x ) {
	var_x = x;
	return formula.value();
}
