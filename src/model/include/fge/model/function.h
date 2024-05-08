#pragma once

#include "fge/model/cache.h"
#include "fge/shared/data.h"
#include "exprtk.hpp"

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


/*******************
 * Function
 ******************/

/**
The output depends on
- The input variable x
- Parameters
- State: values, that
  may be read and written during
  function evaluation.
*/

class Function:
public exprtk::ifunction<C>
{

	public:
		Function();
		virtual ~Function();
		virtual C get(
				const C& x
		) = 0;
		virtual QString toString() const = 0;
		virtual ParameterBindings getParameters() const = 0;
		virtual MaybeError setParameter(
				const QString& name,
				const std::vector<C>& value
		) = 0;
		virtual StateDescriptions getStateDescriptions() const = 0;

		C operator()(const C& x);
};

/*******************
 * FormulaFunction
 ******************/

class Symbols {
	public:
		Symbols(
				const std::map<QString,C>& constants = {},
				const std::map<QString,exprtk::ifunction<C>*>& functions = {}
		);
		void addConstant(
				const QString& name,
				const C& value
		);
		void addFunction(
				const QString& name,
				exprtk::ifunction<C>* function
		);
		symbol_table_t& get();
	private:
		symbol_table_t symbols;
};

class FormulaFunction:
	virtual public Function
{

	public:
		virtual ~FormulaFunction();
		// get:
		virtual C get(
				const C& x
		) override;
		virtual QString toString() const override;
		virtual ParameterBindings getParameters() const override;
		virtual MaybeError setParameter(
				const QString& name,
				const std::vector<C>& value
		) override;
		virtual StateDescriptions getStateDescriptions() const override;

	protected:
		FormulaFunction();
		MaybeError init(
				const QString& formula_str,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescrs,
				const std::vector<Symbols>& additionalSymbols
		);
	friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
			const QString& formulaStr,
			const ParameterBindings& parameters,
			const StateDescriptions& state,
			const std::vector<Symbols>& additionalSymbols
	);

	private:
		QString formulaStr;
		ParameterBindings parameters;
		StateDescriptions stateDescriptions;
		ParameterBindings state;
		expression_t formula;
		C varX;
};

/*******************
 * Fabric method:
 ******************/

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		const ParameterBindings& parameters,
		const StateDescriptions& state,
		const std::vector<Symbols>& additionalSymbols
);
