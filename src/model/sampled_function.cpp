#include "fge/model/sampled_function.h"
#include "fge/model/function_sampling_utils.h"



SampledFormulaFunction::SampledFormulaFunction()
	: Parent()
{}

MaybeError SampledFormulaFunction::init(
		const QString& formula,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescrs,
		const std::vector<Symbols>& additionalSymbols,
		const SamplingSettings& samplingSettings
)
{
	this->samplingSettings = samplingSettings;
	return Parent::init(
			formula,
			parameters,
			stateDescrs,
			additionalSymbols
	);
}

C SampledFormulaFunction::get(
		const C& x
)
{
	return getWithResolution(
			[this](auto x){ return Parent::get(x); },
			x,
			samplingSettings,
			&buffer
	);
}

void SampledFormulaFunction::update()
{
	if( isBufferable( samplingSettings ) ){
		fillBuffer(
				[this](auto x) { return Parent::get(x); },
				samplingSettings,
				&buffer
		);
	}
}
