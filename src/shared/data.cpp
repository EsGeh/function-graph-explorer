#include "fge/shared/data.h"


ParameterDescription descrFromParameters( const ParameterBindings& parameters )
{
	ParameterDescription ret;
	for( auto p : parameters ) {
		ret.push_back( p.first );
	}
	return ret;
}
