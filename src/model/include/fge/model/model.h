#pragma once

#include "fge/model/sampled_func_collection.h"
#include <future>


struct AudioScheduled
{
	virtual ~AudioScheduled() {};

	virtual bool getAudioSchedulingEnabled() const = 0;
	virtual void setAudioSchedulingEnabled(
			const bool value
	) = 0;

	virtual void betweenAudio(
			const PlaybackPosition position,
			const uint samplerate
	) = 0;
};

struct Model:
	public SampledFunctionCollection,
	public AudioScheduled
{
	struct Update {
		std::optional<QString> formula = {};
		std::optional<ParameterBindings> parameters = {};
		std::optional<StateDescriptions> stateDescriptions = {};
		std::optional<bool> playbackEnabled = false;
		std::optional<SamplingSettings> samplingSettings = {};
	};
	using ParameterSignalDone = std::function<void(
		const Index index,
		ParameterBindings parameters
		)>
	;

	virtual ~Model() {};

	virtual MaybeError bulkUpdate(
			const Index index,
			const Update& update
	) = 0;

	virtual void prepareResize() = 0;
	virtual void prepareSet(const Index index) = 0;
	virtual void prepareSetParameterValues(const Index index) = 0;
	virtual void prepareSetIsPlaybackEnabled(const Index index, const bool value) = 0;
	virtual void prepareSetSamplingSettings(const Index index) = 0;
	
	virtual void postSetAny() = 0;

	virtual void scheduleSetParameterValues(
			const Index index,
			const ParameterBindings& parameters,
			ParameterSignalDone signalizeDone
	) = 0;

};

const SamplingSettings no_optimization_settings{
	.resolution = 0,
	.interpolation = 0,
	.periodic = 0,
	.buffered = false
};

std::shared_ptr<Model> modelFactory(
		const SamplingSettings& defSamplingSettings = no_optimization_settings
);
