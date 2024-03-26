#ifndef FUNCTION_H
#define FUNCTION_H

#include "exprtk.hpp"
#include <QString>
#include <memory>
#include <optional>


typedef QString Error;

template <typename T>
using ErrorOrValue = std::variant<Error, T>;

typedef std::optional<Error> MaybeError;

typedef float T;

typedef exprtk::symbol_table<T>
	symbol_table_t;
typedef exprtk::expression<T>
	expression_t;
typedef exprtk::parser<T>
	parser_t;
typedef exprtk::function_compositor<T> 
	compositor_t;
typedef typename compositor_t::function
	function_t;

class Function:
	public exprtk::ifunction<T>
{

	public:
		Function();
		virtual T get( T x ) = 0;
		virtual QString toString() const = 0;

		std::vector<std::pair<T,T>> getPoints(
				const std::pair<T,T>& range
		);
		T operator()(const T& x);
};

class FormulaFunction: public Function
{

	public:
		// get:
		virtual T get( T x );
		virtual QString toString() const;

	private:
		FormulaFunction();
		MaybeError init(
				const QString& formula_str,
				std::vector<symbol_table_t*> additionalSymbols
		);
		friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
				const QString& formulaStr,
				std::vector<symbol_table_t*> additionalSymbols
		);

	private:
		QString formulaStr;
		// symbol_table_t sym_table;
		expression_t formula;
		T varX;

};

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols
);

#endif
