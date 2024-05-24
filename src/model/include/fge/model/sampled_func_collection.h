#pragma once

#include "fge/model/function_collection.h"


using PlaybackPosition = unsigned long int;

/**
Represents the following concepts:

- sampling:
  direct access to functions is
	hidden. The interface only
	allows sampling a function
*/
struct SampledFunctionCollection
{
	using Index = FunctionCollection::Index;
	virtual ~SampledFunctionCollection() {};

	// Size
	virtual uint size() const = 0;
	virtual void resize( const uint size ) = 0;

	// Read entries:
	virtual FunctionParameters get(
			const size_t index
	) const = 0;
	virtual MaybeError getError(
			const Index index
	) const = 0;

	// Write entries:
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

	/***************
	 * Sampling
	 ***************/

	// general settings:
	virtual SamplingSettings getSamplingSettings(
			const Index index
	) const = 0;
	virtual void setSamplingSettings(
			const Index index,
			const SamplingSettings& value
	) = 0;

	// sampling for visual representation:
	virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
			const Index index,
			const std::pair<T,T>& range,
			const unsigned int resolution
	) const = 0;

	// sampling for audio:
	virtual void valuesToBuffer(
			std::vector<float>* buffer,
			const PlaybackPosition position,
			const unsigned int samplerate
	) = 0;
	virtual bool getIsPlaybackEnabled(
			const Index index
	) const = 0;
	virtual void setIsPlaybackEnabled(
			const Index index,
			const bool value
	) = 0;

};

struct SampledFunctionCollectionInternal:
	public SampledFunctionCollection
{
	virtual double getMasterVolume() const = 0;
	virtual void setMasterVolume(const double value) = 0;
};
