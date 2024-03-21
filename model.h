#ifndef MODEL_H
#define MODEL_H

#include "exprtk.hpp"

#include <QString>


typedef float T;

typedef exprtk::symbol_table<T>
	symbol_table_t;
typedef exprtk::expression<T>
	expression_t;
typedef exprtk::parser<T>
	parser_t;

class Model
{

public:
	Model();

	void set(const QString& formula_str);
	T get( T x );

private:
	symbol_table_t sym_table;
	expression_t formula;
	T var_x;

};

#endif // MODEL_H
