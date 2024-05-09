#pragma once

#include "fge/model/model.h"
#include <future>


// 50ms
const double rampTime = 50.0 / 1000.0;

/**
	The model is accessed by 2 threads:

	- the audio worker
	- the gui thread

 The idea:
  synchronize read/write access
	between the 2 threads.
	Always give preference to the
	audio thread if possible.
	Prevent audio xruns at all costs.
	READ access may happen concurrently.
	WRITE access by the gui is deferred
	and executed between the audio
	sampling periods.
*/

class ModelImpl:
	virtual public Model
{
	public:
		using Model::Index;
	public:
		ModelImpl(
				const SamplingSettings& defSamplingSettings = no_optimization_settings
		);

		// Read access functions:
		virtual SamplingSettings getSamplingSettings(
				const Index index
		) override;

		virtual Index size() const override;

		virtual QString getFormula(
				const size_t index
		) const override;

		virtual MaybeError getError(
				const Index index
		) const override;

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;

		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const override;

		virtual void valuesToBuffer(
				std::vector<float>* buffer,
				const PlaybackPosition position,
				const unsigned int samplerate
		) override;

		// setters:
		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) override;
		virtual void resize( const uint size ) override;

		virtual MaybeError set(
				const Index index,
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescriptions
		) override;

		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;

		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;

		// Control Scheduling:

		virtual void setAudioSchedulingEnabled(
				const bool value
		) override;

		virtual void executeWriteOperations(
				const PlaybackPosition position,
				const uint samplerate
		) override;

	private:

		void updateRamps(
				const PlaybackPosition position,
				const uint samplerate
		);

		float audioFunction(
				const PlaybackPosition position,
				const uint samplerate
		);

		std::shared_ptr<FuncNetwork> getNetwork() const {
			return this->network;
		}

	private:
		struct ResizeTask
		{
			uint size;
			using ReturnType =
				decltype(std::declval<FuncNetwork>().resize(size));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetTask
		{
			Index index;
			FunctionParameters parameters;
			using ReturnType =
				decltype(std::declval<FuncNetwork>().set(index,parameters));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetParameterValuesTask
		{
			Index index;
			ParameterBindings parameters;
			using ReturnType =
				decltype(std::declval<FuncNetwork>().setParameterValues(index,parameters));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetIsPlaybackEnabledTask
		{
			Index index;
			bool value;
			using ReturnType =
				decltype(std::declval<ModelImpl>().setIsPlaybackEnabled(index,value));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct RampTask
		{
			Index index;
			double src;
			double dst;
			std::optional<PlaybackPosition> pos = {};
		};
		struct RampMasterTask
		{
			double src;
			double dst;
			std::optional<PlaybackPosition> pos = {};
		};
		struct SignalReturnTask
		{
			std::promise<void> promise;
		};
		using WriteTask = std::variant<
			ResizeTask,
			SetTask,
			SetParameterValuesTask,
			SetIsPlaybackEnabledTask,
			RampTask,
			RampMasterTask,
			SignalReturnTask
		>;
	private:
		bool audioSchedulingEnabled = false;
		PlaybackPosition position = 0;
		double masterVolumeEnv = 1;
		std::map<Index,double> volumeEnvelopes;
		bool currentEnvTaskDone = false;
		mutable std::mutex networkLock;
		std::shared_ptr<FuncNetwork> network;
		mutable std::mutex tasksLock;
		std::deque<WriteTask> writeTasks;
};
