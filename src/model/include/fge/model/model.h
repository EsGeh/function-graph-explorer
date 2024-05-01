#pragma once

#include "fge/model/function.h"
#include "fge/shared/data.h"
#include <memory>
#include <optional>


typedef
	ErrorOrValue<std::shared_ptr<Function>>
	ErrorOrFunction
;

struct FunctionEntry {
	QString formula;
	SamplingSettings samplingSettings;
	ErrorOrFunction errorOrFunction = std::unexpected("not yet initialized");
	// StateDescriptions stateDescriptions;
};

const SamplingSettings no_optimization_settings{
	.resolution = 0,
	.interpolation = 0,
	.caching = false
};

class Model
{
	public:
		Model(
				const SamplingSettings& defSamplingSettings = no_optimization_settings
		);

		// get:
		size_t size() const;

		QString getFormula(
				const size_t index
		) const;
		MaybeError getError(
				const size_t index
		) const;
		SamplingSettings getSamplingSettings(
				const size_t index
		) const;
		void setSamplingSettings(
				const size_t index,
				const SamplingSettings& value
		);
		ErrorOrValue<ParameterBindings> getParameters(
				const size_t index
		) const;
		MaybeError setParameterValues(
				const size_t index,
				const ParameterBindings& parameters
		);
		ErrorOrValue<StateDescriptions> getStateDescriptions(
				const size_t index
		) const;
		ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const size_t index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const;
		MaybeError valuesToAudioBuffer(
				const size_t index,
				std::vector<float>* buffer,
				const T duration,
				const T speed,
				const T offset,
				const unsigned int samplerate,
				std::function<float(const double)> volumeFunction
		) const;

		// set:
		void resize( const size_t size );
		MaybeError set(
				const size_t index,
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescriptions
		);
	
		ErrorOrFunction getFunction(const size_t index) const;
	private:
		void updateFormulas(
				const size_t startIndex,
				const std::optional<ParameterBindings>& setBindings,
				const std::optional<StateDescriptions>& setStateDescriptions
		);

	private:
		Symbols constants;
		// symbol_table_t constantSymbols;
		std::vector<std::shared_ptr<FunctionEntry>> functions;
		SamplingSettings defSamplingSettings;

};
