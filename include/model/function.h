#ifndef FUNCCION_H
#define FUNCCION_H

#include "global.h"
#include "exprtk.hpp"
#include <QString>
#include <memory>
#include <optional>


typedef exprtk::symbol_table<C>
	symbol_table_t;
typedef exprtk::expression<C>
	expression_t;
typedef exprtk::parser<C>
	parser_t;
typedef exprtk::function_compositor<C> 
	compositor_t;
typedef typename compositor_t::function
	function_t;


class Function:
	public exprtk::ifunction<C>
{

	public:
		Function();
		virtual C get( C x ) = 0;
		virtual QString toString() const = 0;

		C operator()(const C& x);
};

class FormulaFunction: public Function
{

	public:
		// get:
		virtual C get( C x );
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
		C varX;

};

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols
);

#endif
