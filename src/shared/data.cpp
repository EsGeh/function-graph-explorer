#include "fge/shared/data.h"


std::vector<QString> descrFromParameters( const ParameterBindings& parameters )
{
	std::vector<QString> ret;
	for( auto p : parameters ) {
		ret.push_back( p.first );
	}
	return ret;
}
