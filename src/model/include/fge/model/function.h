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
				const C& value
		) = 0;
		virtual StateDescriptions getStateDescriptions() const = 0;

		C operator()(const C& x);

		virtual SamplingSettings getSamplingSettings() const = 0;
		virtual void setSamplingSettings(const SamplingSettings& samplingSettings) = 0;

		virtual void update() = 0;
		virtual void resetState() = 0;
};

/*******************
 * FormulaFunction
 ******************/

class Symbols {
	public:
		using function_t = std::variant<
			exprtk::igeneric_function<C>*,
			exprtk::ifunction<C>*
		>;
	public:
		Symbols(
				const std::map<QString,C>& constants = {},
				const std::map<QString, function_t>& functions = {}
		);
		void addConstant(
				const QString& name,
				const C& value
		);
		void addFunction(
				const QString& name,
				function_t function
		);
		symbol_table_t& get();
	private:
		symbol_table_t symbols;
};

const SamplingSettings no_optimization_settings{
	.resolution = 0,
	.interpolation = 0,
	.periodic = 0,
	.buffered = false
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
				const C& value
		) override;
		virtual StateDescriptions getStateDescriptions() const override;

		virtual void resetState() override;
		virtual void update() override {};

		virtual SamplingSettings getSamplingSettings() const override {
			return no_optimization_settings;
		};
		virtual void setSamplingSettings(const SamplingSettings& samplingSettings) override {};

	protected:
		FormulaFunction();
		MaybeError init(
				const QString& formula_str,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescrs,
				const std::vector<Symbols>& additionalSymbols
		);

	private:
		QString formulaStr;
		ParameterBindings parameters;
		StateDescriptions stateDescriptions;
		StateBindings state;
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
		const std::vector<Symbols>& additionalSymbols,
		const SamplingSettings& samplingSettings
);
