#pragma once

#include "fge/model/model.h"
#include "func_network.h"
#include "func_network_impl.h"
#include <future>
#include <memory>
#include <optional>


// 50ms
const double rampTime = 50.0 / 1000.0;

C getWithResolution(
		std::shared_ptr<Function> function,
		const C& x,
		const SamplingSettings& samplingSettings,
		std::shared_ptr<Cache> cache
);

C interpolate(
		const C& x,
		const std::vector<C> ys,
		const int shift,
		const uint resolution

);

int xToRasterIndex(
		const C& x,
		const uint resolution
);

C rasterIndexToY(
		std::shared_ptr<Function> function,
		int x,
		const uint resolution
);

/**
	The model is accessed by 2 threads:

	- the audio worker
	- the gui thread

 The idea:
  synchronize read/write access
	between the 2 threads.
	Always give preference to the
	audio thread if possible.
	Prevent audio xruns at all costs. READ access may happen concurrently.
	WRITE access by the gui is deferred
	and executed between the audio
	sampling periods.
*/

struct NodeInfo:
	public FunctionCollectionWithInfo::NodeInfo
{
	bool isPlaybackEnabled = false;
	double volumeEnvelope = 1;
	SamplingSettings samplingSettings;
	mutable std::shared_ptr<Cache> cache;
};

using AudioCallback = std::function<void(
		const PlaybackPosition position,
		const uint samplerate
)>;



class SampledFunctionCollectionImpl:
	public SampledFunctionCollectionInternal,
	private FunctionCollectionImpl
{
	public:
		using LowLevel = FunctionCollectionImpl;
		using SampledFunctionCollectionInternal::Index;
	public:
		SampledFunctionCollectionImpl(
				const SamplingSettings& defSamplingSettings,
				AudioCallback audioCallback 
		);
	
		// Size:
		uint size() const override { return LowLevel::size(); }
		void resize(const uint size) override { LowLevel::resize(size); }

		// Read entries:
		virtual QString getFormula(
				const size_t index
		) const override;

		virtual MaybeError getError(
				const Index index
		) const override;

		// Set Entries:

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


		/***************
		 * Sampling
		 ***************/

		// general settings:
		virtual SamplingSettings getSamplingSettings(
				const Index index
		) override;
		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) override;

		// sampling for visual representation:
		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const override;

		// sampling for audio:
		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;
		virtual void valuesToBuffer(
				std::vector<float>* buffer,
				const PlaybackPosition position,
				const unsigned int samplerate
		) override;

		virtual double getMasterVolume() const override;
		virtual void setMasterVolume(const double value) override;

	public:
		::NodeInfo* getNodeInfo( const Index index ) {
			return static_cast<::NodeInfo*>(LowLevel::getNodeInfo(index));
		}

	private:
		float audioFunction(
				const PlaybackPosition position,
				const uint samplerate
		);
	private:
		virtual std::shared_ptr<LowLevel::NodeInfo> createNodeInfo(
				const Index index,
				std::shared_ptr<Function> maybeFunction
		) override;

		void updateCaches( const Index index );
		std::shared_ptr<Cache> updateCache( std::shared_ptr<Function> maybeFunction );

		const ::NodeInfo* getNodeInfoConst( const Index index ) const {
			return static_cast<::NodeInfo*>(LowLevel::getNodeInfo(index));
		}

	private:
		SamplingSettings defSamplingSettings;
		AudioCallback audioCallback;
		double masterVolume = 1;
};

class ScheduledFunctionCollectionImpl:
	public Model
{
	public:
		ScheduledFunctionCollectionImpl(
				const SamplingSettings& defSamplingSettings
		);

		virtual Index size() const override;
		virtual void resize( const uint size ) override;

		// Read entries:
		virtual QString getFormula(
				const size_t index
		) const override;

		virtual MaybeError getError(
				const Index index
		) const override;

		// Set Entries:

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


		/***************
		 * Sampling
		 ***************/

		// general settings:
		virtual SamplingSettings getSamplingSettings(
				const Index index
		) override;
		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) override;

		// sampling for visual representation:
		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const override;

		// sampling for audio:
		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;
		virtual void valuesToBuffer(
				std::vector<float>* buffer,
				const PlaybackPosition position,
				const unsigned int samplerate
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
		struct ResizeTask
		{
			uint size;
			using ReturnType =
				decltype(std::declval<SampledFunctionCollectionInternal>().resize(size));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetTask
		{
			Index index;
			FunctionParameters parameters;
			using ReturnType =
				decltype(std::declval<SampledFunctionCollectionInternal>().set(index,parameters.formula, parameters.parameters, parameters.stateDescriptions));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetParameterValuesTask
		{
			Index index;
			ParameterBindings parameters;
			using ReturnType =
				decltype(std::declval<SampledFunctionCollectionInternal>().setParameterValues(index,parameters));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetIsPlaybackEnabledTask
		{
			Index index;
			bool value;
			using ReturnType =
				decltype(std::declval<SampledFunctionCollectionInternal>().setIsPlaybackEnabled(index,value));
			PlaybackPosition pos = 0;
			std::promise<ReturnType> promise;
		};
		struct SetSamplingSettingsTask
		{
			Index index;
			SamplingSettings value;
			using ReturnType =
				decltype(std::declval<SampledFunctionCollectionInternal>().setSamplingSettings(index,value));
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
			SetSamplingSettingsTask,
			RampTask,
			RampMasterTask,
			SignalReturnTask
		>;

	private:
		void updateRamps(
				const PlaybackPosition position,
				const uint samplerate
		);
		std::shared_ptr<SampledFunctionCollectionImpl> getNetwork() const {
			return this->network;
		}

	private:
		bool audioSchedulingEnabled = false;
		PlaybackPosition position = 0;
		bool currentEnvTaskDone = false;
		mutable std::mutex networkLock;
		std::shared_ptr<SampledFunctionCollectionImpl> network;
		mutable std::mutex tasksLock;
		std::deque<WriteTask> writeTasks;
};

using ModelImpl = ScheduledFunctionCollectionImpl;
