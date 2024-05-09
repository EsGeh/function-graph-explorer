#pragma once

#include "fge/model/func_network.h"


/**
- hide direct access to functions
*/
class FuncNetworkHighLevel
{
	public:
		using Index = FuncNetwork::Index;
		using PlaybackPosition = unsigned long int;
	public:
		virtual ~FuncNetworkHighLevel() {};
		// Size
		virtual uint size() const = 0;
		virtual void resize( const uint size ) = 0;

		// Read access functions:
		virtual QString getFormula(
				const size_t index
		) const = 0;
		virtual MaybeError getError(
				const Index index
		) const = 0;
		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const = 0;

		virtual void valuesToBuffer(
				std::vector<float>* buffer,
				const PlaybackPosition position,
				const unsigned int samplerate
		) = 0;
		virtual bool getIsPlaybackEnabled(
				const Index index
		) const = 0;
		virtual SamplingSettings getSamplingSettings(
				const Index index
		) = 0;

		// Write access functions:

		virtual MaybeError set(
				const Index index,
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescriptions
		) = 0;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) = 0;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) = 0;
		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) = 0;
};

class AudioScheduled:
	virtual public FuncNetworkHighLevel
{
	public:
		virtual void setAudioSchedulingEnabled(
				const bool value
		) = 0;

		virtual void executeWriteOperations(
				const PlaybackPosition position,
				const uint samplerate
		) = 0;
};

class Model:
	virtual public FuncNetworkHighLevel,
	virtual public AudioScheduled
{
};

const SamplingSettings no_optimization_settings{
	.resolution = 0,
	.interpolation = 0,
	.caching = false
};

std::shared_ptr<Model> modelFactory(
		const SamplingSettings& defSamplingSettings = no_optimization_settings
);
