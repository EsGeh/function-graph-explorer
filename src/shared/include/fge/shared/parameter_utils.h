#pragma once

#include "fge/shared/data.h"


struct FunctionDataDescription {
	ParameterDescriptions parameterDescriptions;
	StateDescriptions stateDescriptions;
};

FunctionDataDescription parseFunctionDataDescription(
		const QString& str
);

QString functionDataDescriptionToString( const FunctionDataDescription& dataDescription );
