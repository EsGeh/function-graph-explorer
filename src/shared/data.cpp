#include "fge/shared/data.h"


ParameterDescriptions descrFromParameters( const ParameterBindings& parameters )
{
	ParameterDescriptions ret;
	for( auto p : parameters ) {
		ret.push_back( p.first );
	}
	return ret;
}
