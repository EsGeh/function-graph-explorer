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

	// get:
	bool getIsValidExpression() const;
	std::vector<std::pair<T,T>> getPoints();
	T getXMin();
	T getXMax();

	// set:
	void set(const QString& formula_str);
	void setXMin(T value);
	void setXMax(T value);
	void resetXMin();
	void resetXMax();

private:
	symbol_table_t sym_table;
	expression_t formula;
	bool isValidExpression;
	T varX;
	T xMin, xMax;

	T get( T x );
};

#endif // MODEL_H
