#ifndef MODEL_H
#define MODEL_H

#include "function.h"
#include <QString>
#include <memory>
#include <optional>


typedef
	ErrorOrValue<std::shared_ptr<Function>>
	ErrorOrFunction
;

struct FunctionEntry {
	QString string;
	ErrorOrFunction errorOrFunction;
};

/*
struct FunctionWrap: public exprtk::ifunction<T>
{
	FunctionWrap(
			Function* function
	)
		: exprtk::ifunction<T>(1)
		, function( function )
	{}

	T operator()(const T& x)
	{
		 return function->get( x );
	}
	private:
		Function* function;
};
*/

class Model
{
	public:
		Model();

		// get:
		size_t size() const;
		ErrorOrFunction get(const size_t index);

		// set:
		void resize( const size_t size );
		ErrorOrFunction set( const size_t index, const QString& functionStr );
	
	private:
		void updateFormulas( const size_t startIndex );

	private:
		symbol_table_t constantSymbols;
		symbol_table_t functionSymbols;;
		std::vector<std::shared_ptr<FunctionEntry>> functions;
};

#endif // MODEL_H
