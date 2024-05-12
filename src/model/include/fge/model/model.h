#pragma once

#include "fge/model/sampled_func_collection.h"


struct AudioScheduled
{
	virtual ~AudioScheduled() {};

	virtual void setAudioSchedulingEnabled(
			const bool value
	) = 0;

	virtual void executeWriteOperations(
			const PlaybackPosition position,
			const uint samplerate
	) = 0;
};

struct Model:
	public SampledFunctionCollection,
	public AudioScheduled
{
	virtual ~Model() {};
};

const SamplingSettings no_optimization_settings{
	.resolution = 0,
	.interpolation = 0,
	.caching = false
};

std::shared_ptr<Model> modelFactory(
		const SamplingSettings& defSamplingSettings = no_optimization_settings
);
