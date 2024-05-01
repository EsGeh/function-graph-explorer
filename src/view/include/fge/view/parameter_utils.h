#pragma once

#include "fge/shared/data.h"


struct ParameterDescription {
	double initial = 0;
	double min = 0;
	double max = 1;
	double step = 0;
};

using StateDescription = VariableDescription;

struct FunctionDataDescription {
	std::map<QString,ParameterDescription> parameterDescriptions;
	std::map<QString,StateDescription> stateDescriptions;
};


FunctionDataDescription parseFunctionDataDescription(
		const QString& str
);

QString functionDataDescriptionToString( const FunctionDataDescription& dataDescription );

