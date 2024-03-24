#ifndef MODEL_H
#define MODEL_H

#include "exprtk.hpp"
#include <QString>
#include <memory>
#include <optional>


typedef float T;

typedef exprtk::symbol_table<T>
	symbol_table_t;
typedef exprtk::expression<T>
	expression_t;
typedef exprtk::parser<T>
	parser_t;


class Function {

	public:
		virtual T get( T x ) = 0;
		virtual QString toString() const = 0;

		std::vector<std::pair<T,T>> getPoints(
				const std::pair<T,T>& range
		);
};

class FormulaFunction: public Function
{

	public:
		// get:
		virtual T get( T x );
		virtual QString toString() const;

	private:
		FormulaFunction();
		bool init(const QString& formula_str);
		friend std::optional<std::shared_ptr<Function>> formulaFunctionFactory(
			const QString& formulaStr
		);

	private:
		QString formulaStr;
		symbol_table_t sym_table;
		expression_t formula;
		T varX;

};

std::optional<std::shared_ptr<Function>> formulaFunctionFactory(
	const QString& formulaStr
);


typedef std::vector<std::optional<std::shared_ptr<Function>>> Model;

#endif // MODEL_H
