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
	bool isAudioEnabled = false;
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
		size_t size();

		QString getFormula(
				const size_t index
		);
		MaybeError getError(
				const size_t index
		);
		SamplingSettings getSamplingSettings(
				const size_t index
		);
		void setSamplingSettings(
				const size_t index,
				const SamplingSettings& value
		);
		ErrorOrValue<ParameterBindings> getParameters(
				const size_t index
		);
		MaybeError setParameterValues(
				const size_t index,
				const ParameterBindings& parameters
		);
		ErrorOrValue<StateDescriptions> getStateDescriptions(
				const size_t index
		);
		ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const size_t index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		);
		float audioFunction(
				const unsigned long int position,
				const uint samplerate
		);
		
		bool getIsPlaybackEnabled(
				const uint index
		);
		void setIsPlaybackEnabled(
				const uint index,
				const bool value
		);

		// set:
		void resize( const size_t size );
		MaybeError set(
				const size_t index,
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescriptions
		);
	
		ErrorOrFunction getFunction(const size_t index);
	private:
		MaybeError getErrorPrivate(
				const size_t index
		) const;
		ErrorOrFunction getFunctionPrivate(const size_t index) const;
		void updateFormulas(
				const size_t startIndex,
				const std::optional<ParameterBindings>& setBindings,
				const std::optional<StateDescriptions>& setStateDescriptions
		);

	private:
		std::mutex lock;
		Symbols constants;
		std::vector<std::shared_ptr<FunctionEntry>> functions;
		SamplingSettings defSamplingSettings;
};
