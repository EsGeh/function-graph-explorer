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


class Function:
	public exprtk::ifunction<C>
{

	public:
		Function();
		virtual ~Function();
		virtual C get( C x ) = 0;
		virtual QString toString() const = 0;

		C operator()(const C& x);
};

class FormulaFunction:
	virtual public Function
{

	public:
		virtual ~FormulaFunction();
		// get:
		virtual C get( C x );
		virtual QString toString() const;


	protected:
		FormulaFunction();
		MaybeError init(
				const QString& formula_str,
				std::vector<symbol_table_t*> additionalSymbols
		);
		friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
				const QString& formulaStr,
				std::vector<symbol_table_t*> additionalSymbols,
				const uint resolution,
				const bool enableInterpolate
		);

	private:
		QString formulaStr;
		// symbol_table_t sym_table;
		expression_t formula;
		C varX;

};

class CachedFunction:
	virtual public Function,
	protected FormulaFunction
{
	public:
		virtual ~CachedFunction();
		virtual C get( C x );
	protected:
		CachedFunction(
				const uint resolution, // 0 means: no caching
				const bool enableInterpolate
		);
		friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
				const QString& formulaStr,
				std::vector<symbol_table_t*> additionalSymbols,
				const uint resolution,
				const bool enableInterpolate
		);
		// simple linear interpolation
		C interpolate(
				const C& x,
				const C ys[2]
		);
	private:
		const uint resolution;
		bool enableInterpolate;
		typedef int CacheIndex;
		std::function<C(CacheIndex)> function;
		Cache cache;
};

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory_internal(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution,
		const bool enableInterpolate
);

ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
		const QString& formulaStr,
		std::vector<symbol_table_t*> additionalSymbols,
		const uint resolution = 0,
		const bool enableInterpolate = false
);

#endif
