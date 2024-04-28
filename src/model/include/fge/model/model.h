#pragma once

#include "fge/model/function.h"
#include "fge/shared/data.h"
#include <memory>


typedef
	ErrorOrValue<std::shared_ptr<Function>>
	ErrorOrFunction
;

struct FunctionEntry {
	QString string;
	SamplingSettings samplingSettings;
	ParameterBindings parameters;
	ErrorOrFunction errorOrFunction;
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
		ParameterBindings getParameters(
				const size_t index
		) const;
		void setParameters(
				const size_t index,
				const ParameterBindings& parameters
		);
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
		ErrorOrFunction getFunction(const size_t index) const;

		// set:
		void resize( const size_t size );
		MaybeError set( const size_t index, const QString& functionStr );
	
	private:
		void updateFormulas( const size_t startIndex );

	private:
		symbol_table_t constantSymbols;
		symbol_table_t functionSymbols;;
		std::vector<std::shared_ptr<FunctionEntry>> functions;
		SamplingSettings defSamplingSettings;

};
