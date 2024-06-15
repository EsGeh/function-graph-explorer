#pragma once

#include "fge/model/function.h"
#include "fge/model/cache.h"

/*******************
 * SampledFunction
 ******************/

class SampledFormulaFunction:
	public FormulaFunction
{
	public:
		using Parent = FormulaFunction;
	public:
		virtual C get(
				const C& x
		) override;

		virtual void update() override;

		virtual SamplingSettings getSamplingSettings() const override {
			return samplingSettings;
		}
		virtual void setSamplingSettings(const SamplingSettings& samplingSettings) override
			{ this->samplingSettings = samplingSettings; }

	protected:
		SampledFormulaFunction();
		MaybeError init(
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescrs,
				const std::vector<Symbols>& additionalSymbols,
				const SamplingSettings& samplingSettings
		);
	private:
		SamplingSettings samplingSettings;
		FunctionBuffer buffer;
	public:
	friend ErrorOrValue<std::shared_ptr<Function>> formulaFunctionFactory(
			const QString& formulaStr,
			const ParameterBindings& parameters,
			const StateDescriptions& state,
			const std::vector<Symbols>& additionalSymbols,
			const SamplingSettings& samplingSettings
	);
};
