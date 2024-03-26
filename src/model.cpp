#include "model.h"
#include <QDebug>
#include <stdexcept>


inline QString functionName( const size_t index ) {
	return QString("f%1").arg( index );
}

Model::Model()
	: constantSymbols(symbol_table_t::symtab_mutability_type::e_immutable)
	, functionSymbols(symbol_table_t::symtab_mutability_type::e_immutable)
	, functions()
{
	constantSymbols.add_constant( "pi", acos(-1) );
}

size_t Model::size() const {
	return functions.size();
}

ErrorOrFunction Model::get(const size_t index) {
	return functions.at( index )->errorOrFunction;
}

void Model::resize( const size_t size ) {
	const auto oldSize = functions.size();
	if( size < oldSize ) {
		functions.resize( size );
	}
	else if( size > oldSize ) {
		for( auto i=oldSize; i<size; i++ ) {
			auto entry = std::shared_ptr<FunctionEntry>(new FunctionEntry {
				(i>0)
					? QString("%1(x)").arg( functionName(i-1) )
					: "x",
				{}
			});
			functions.push_back( entry );
		}
		updateFormulas(oldSize);
	}
	assert( this->size() == size );
}

ErrorOrFunction Model::set( const size_t index, const QString& functionStr ) {
	// assert( index < size() );
	auto entry = functions[index];
	entry->string = functionStr;
	updateFormulas( index );
	return entry->errorOrFunction;
}

void Model::updateFormulas(const size_t startIndex) {
	functionSymbols.clear();
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = functions.at(i);
		if( entry->errorOrFunction.index() == 1 ) {
			functionSymbols.add_function(
					functionName( i ).toStdString().c_str(),
					*(std::get<std::shared_ptr<Function>>(
							entry->errorOrFunction
					).get())
			);
		}
	}
	/* update entries
	 * from startIndex:
	 */
	for( size_t i=startIndex; i<functions.size(); i++ ) {
		auto entry = functions.at(i);
		entry->errorOrFunction = formulaFunctionFactory(
				entry->string,
				{
					&constantSymbols,
					&functionSymbols
				}
		);
		if( entry->errorOrFunction.index() == 1 ) {
			functionSymbols.add_function(
					functionName( i ).toStdString().c_str(),
					*(std::get<std::shared_ptr<Function>>( entry->errorOrFunction ).get())
			);
		}
	}
}
