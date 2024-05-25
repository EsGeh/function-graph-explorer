#pragma once

#include "fge/shared/utils.h"


class FunctionBuffer
{
	public:
		typedef int Index;
		typedef C Value;
	public:
		inline std::pair<Index,Index> getRange() const {
			return { indexMin, indexMin+buffer.size() };
		}
		inline void fill(
				const Index indexMin,
				const uint size,
				std::function<Value(const Index)> function
		) {
			this->indexMin = indexMin;
			buffer.resize( size );
			for( uint i=0; i<buffer.size(); i++ ) {
				buffer[i] = function( indexMin+i );
			}
		}
		inline bool inRange(const Index index) const {
			auto range = getRange();
			return index >= range.first
				&& index < range.second
			;
		}
		inline Value lookup(
				const Index index
		) const {
			return buffer[indexMin+index];
		}
	private:
		Index indexMin;
		std::vector<Value> buffer;
};
