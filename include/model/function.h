#ifndef FUNCTION_H
#define FUNCTION_H

#include "cache.h"
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
		virtual C get( C x ) = 0;
		virtual QString toString() const = 0;
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
		// remember: degree needs k+1 points
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

class FormulaFunction:
	virtual public Function
{

	public:
		virtual ~FormulaFunction();
		// get:
		virtual C get( C x ) override;
		virtual QString toString() const override;

	protected:
		FormulaFunction();
		MaybeError init(
				const QString& formula_str,
				std::vector<symbol_table_t*> additionalSymbols
		);

	private:
		QString formulaStr;
		// symbol_table_t sym_table;
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
				const uint resolution,
				const uint interpolation,
				const bool caching
		);
		virtual C get( C x ) override;

		uint getResolution() const override;
		void setResolution(const uint value) override;

		uint getInterpolation() const override;
		void setInterpolation(const uint value) override;

		virtual bool getCaching() const override;
		virtual void setCaching(const bool value) override;
	protected:
		typedef int CacheIndex;
		C interpolate(
				const C& x,
				const std::vector<C> ys,
				const int shift
		) const;
		inline CacheIndex xToCacheIndex(const C& x) {
				return std::floor(x.c_.real() * getResolution());
		};
		inline C cacheIndexToY(int x) {
			return FormulaFunction::get( C(T(x) / getResolution(),0) );
		};
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
				const uint resolution,
				const uint interpolation,
				const bool enableCaching
		);
	friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
			const QString& formulaStr,
			std::vector<symbol_table_t*> additionalSymbols,
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
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution,
		const uint interpolation,
		const bool enableCaching
);

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution = 0,
		const uint interpolation = 0,
		const bool enableCaching = true 
);

#endif
