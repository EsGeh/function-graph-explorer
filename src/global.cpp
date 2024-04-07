#include "global.h"

QString to_qstring(const C& value) 
{
	return QString("(%1+%2i)")
		.arg( value.c_.real() )
		.arg( value.c_.imag() );
}

QString to_qstring(const std::pair<C,C>& value)
{
	return QString("(%1,%2)")
		.arg( to_qstring( value.first ) )
		.arg( to_qstring( value.second ) );
}
