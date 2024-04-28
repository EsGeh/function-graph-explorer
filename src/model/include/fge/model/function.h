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

class Function:
	public exprtk::ifunction<C>
{

	public:
		Function();
		virtual ~Function();
		virtual C get(
				const C& x,
				const ParameterBindings& parameters
		) = 0;
		virtual QString toString() const = 0;
		virtual ParameterDescription getParameterDescription() const = 0;
		/* Quantization of input values.
		 *   0: no quantization. infinite resolution.
		 */
		virtual uint getResolution() const = 0;
		virtual void setResolution(const uint value) = 0;
		/* polynomial interpolation for values
		 * 	 0: no interpolation
		 * 	   y[x] = y[floor(x*resolution)/resolution]
		 * 	 1: linear interpolation (2 points)
		 * 	 2: quadratic interpolation (3 points)
		 * 	 ...
		 */
		// remember: degree k needs k+1 points
		virtual uint getInterpolation() const = 0;
		virtual void setInterpolation(const uint value) = 0;
		// caching:
		virtual bool getCaching() const = 0;
		virtual void setCaching(const bool value) = 0;

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
				const C& x,
				const ParameterBindings& parameters
		) override;
		virtual QString toString() const override;
		virtual ParameterDescription getParameterDescription() const override;

	protected:
		FormulaFunction(
				const std::vector<QString>& parameters = {}
		);
		MaybeError init(
				const QString& formula_str,
				const std::vector<Symbols>& additionalSymbols
		);

	private:
		QString formulaStr;
		ParameterBindings parameters;
		expression_t formula;
		C varX;


};

/*******************
 * FunctionWithResolution
 ******************/

class FunctionWithResolution:
	public FormulaFunction
{
	public:
		FunctionWithResolution(
				const std::vector<QString>& parameters,
				const uint resolution,
				const uint interpolation,
				const bool caching
		);
		virtual C get(
				const C& x,
				const ParameterBindings& parameters
		) override;

		uint getResolution() const override;
		void setResolution(const uint value) override;

		uint getInterpolation() const override;
		void setInterpolation(const uint value) override;

		virtual bool getCaching() const override;
		virtual void setCaching(const bool value) override;
	protected:
		typedef int RasterIndex;
		C interpolate(
				const C& x,
				const std::vector<C> ys,
				const int shift
		) const;
		RasterIndex xToRasterIndex(const C& x);
		C rasterIndexToY(
				int x,
				const ParameterBindings& parameters
		);
	private:
		uint resolution;
		uint interpolation;
		// Caching:
		bool caching = true;
		Cache cache;
};

/*******************
 * FunctionImpl
 ******************/

class FunctionImpl:
	virtual public Function,
	public FunctionWithResolution
{
	protected:
		FunctionImpl(
				const std::vector<QString>& parameters,
				const uint resolution,
				const uint interpolation,
				const bool enableCaching
		);
	friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
			const QString& formulaStr,
			const std::vector<QString>& parameters,
			const std::vector<Symbols>& additionalSymbols,
			const uint resolution,
			const uint enableInterpolate,
			const bool enableCaching
	);
};

/*******************
 * Fabric method:
 ******************/

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
		const QString& formulaStr,
		const std::vector<QString>& parameters,
		const std::vector<Symbols>& additionalSymbols,
		const uint resolution,
		const uint interpolation,
		const bool enableCaching
);

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		const std::vector<QString>& parameters,
		const std::vector<Symbols>& additionalSymbols,
		const uint resolution = 0,
		const uint interpolation = 0,
		const bool enableCaching = true 
);
