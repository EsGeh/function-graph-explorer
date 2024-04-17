#pragma once

#include "global.h"
#include <deque>
#include <optional>


class Cache
{
	public:
		typedef int Index;
		typedef C Value;
	public:
		inline Cache(
			std::function<Value(Index)> function
		);
		inline std::pair<Cache::Value,bool> lookup( const Index index );
		inline void clear();
	private:
		std::function<Value(Index)> function;
		Index indexMin;
		std::deque<std::optional<Value>> buffer;
};

inline Cache::Cache(
	std::function<Value(Index)> function
)
	: function(function)
	, indexMin(0)
	, buffer()
{}

inline std::pair<Cache::Value,bool> Cache::lookup( const Index index ) {
	if( buffer.size() == 0 ) {
		indexMin = index;
	}
	Index lookupPosition = index - indexMin;
	// resize buffer, if necessary:
	{
		// index is greater than
		// last buffered value:
		if( lookupPosition > int(buffer.size()) - 1 ) {
			buffer.resize( lookupPosition+1 );
		}
		// index is smaller than
		// first buffered value:
		else if( lookupPosition < 0 ) {
			buffer.insert( buffer.begin(), -lookupPosition, {} );
			indexMin = index;
			lookupPosition = 0;
		}
	}
	// from here, the buffer
	// contains an entry 
	// for lookupPosition
	auto& lookupRes = buffer[lookupPosition];
	if( lookupRes.has_value() ) {
		return { lookupRes.value(), true };
	}
	// qDebug().nospace() << this->toString() << ": cache fail at " << index;
	lookupRes = function( index );
	assert( buffer[lookupPosition].has_value() );
	return { lookupRes.value(), false };
}

inline void Cache::clear()
{
	buffer.clear();
}
