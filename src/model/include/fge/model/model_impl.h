#pragma once

#include "fge/model/model.h"
#include "sampled_func_collection_impl.h"
#include "template_utils.h"
// #include <future>
#include <memory>
#include <optional>
#include <tuple>
#include <semaphore>


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
	Prevent audio xruns at all costs. READ access may happen concurrently.
	WRITE access by the gui is deferred
	and executed between the audio
	sampling periods.
*/

constexpr auto resize = &SampledFunctionCollectionImpl::resize;
constexpr auto set = &SampledFunctionCollectionImpl::set;
constexpr auto setParameterValues = &SampledFunctionCollectionImpl::setParameterValues;
constexpr auto setIsPlaybackEnabled = &SampledFunctionCollectionImpl::setIsPlaybackEnabled;
constexpr auto setSamplingSettings = &SampledFunctionCollectionImpl::setSamplingSettings;

constexpr auto setters = std::make_tuple(
		std::make_pair(resize,"resize"),
		std::make_pair(set,"set"),
		std::make_pair(setParameterValues,"setParameterValues"),
		std::make_pair(setIsPlaybackEnabled,"setIsPlaybackEnabled"),
		std::make_pair(setSamplingSettings, "setSamplingSettings")
);

using ResizeTask = SetterTask<resize>;
using SetTask = SetterTask<set>;
using SetParameterValuesTask = SetterTask<setParameterValues>;
using SetIsPlaybackEnabledTask = SetterTask<setIsPlaybackEnabled>;
using SetSamplingSettingsTask = SetterTask<setSamplingSettings>;


class ScheduledFunctionCollectionImpl:
	public Model
{
	public:
		ScheduledFunctionCollectionImpl(
				const SamplingSettings& defSamplingSettings
		);
		~ScheduledFunctionCollectionImpl();

		// READ:

		virtual Index size() const override;

		virtual FunctionParameters get(
				const size_t index
		) const override;

		virtual MaybeError getError(
				const Index index
		) const override;

		virtual SamplingSettings getSamplingSettings(
				const Index index
		) const override;

		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const override;

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;

		// WRITE:

		virtual void prepareResize() override;
		virtual void prepareSet(const Index index) override;
		virtual void prepareSetParameterValues(const Index index) override;
		virtual void prepareSetIsPlaybackEnabled(const Index index, const bool value) override;
		virtual void prepareSetSamplingSettings(const Index index) override;

		virtual void postSetAny() override;

		virtual MaybeError bulkUpdate(
				const Index index,
				const Update& update
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

		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) override;

		// Control Scheduling:

		virtual void setAudioSchedulingEnabled(
				const bool value
		) override;

		virtual void valuesToBuffer(
				std::vector<float>* buffer,
				const PlaybackPosition position,
				const unsigned int samplerate
		) override;

		virtual void betweenAudio(
				const PlaybackPosition position,
				const uint samplerate
		) override;

	private:
		struct RampTask
		{
			Index index;
			double src;
			double dst;
			std::optional<PlaybackPosition> pos = {};
			bool done = false;
		};
		struct RampMasterEnvTask
		{
			double src;
			double dst;
			std::optional<PlaybackPosition> pos = {};
			bool done = false;
		};
		struct RampMasterVolumeTask
		{
			double src;
			double dst;
			std::optional<PlaybackPosition> pos = {};
			bool done = false;
		};
		struct SignalReturnTask
		{
			std::promise<void> promise;
			bool done = false;
		};
		using WriteTask = std::variant<
			ResizeTask,
			SetTask,
			SetParameterValuesTask,
			SetIsPlaybackEnabledTask,
			SetSamplingSettingsTask,
			RampTask,
			RampMasterEnvTask,
			RampMasterVolumeTask,
			SignalReturnTask
		>;

	private:

		void prepareResizeImpl();
		void prepareSetImpl(const Index index);
		void prepareSetParameterValuesImpl(const Index index);
		void prepareSetIsPlaybackEnabledImpl(const Index index, const bool value);
		void prepareSetSamplingSettingsImpl(const Index index);

		void postSetAnyImpl();

		void updateMasterVolumeImpl();

		/** Called by the audio thread
		from `valuesToBuffer` at samplerate.
		Therefore implicitly run with
		the same locks as `valuesToBuffer`
		*/
		void updateRamps(
				const PlaybackPosition position,
				const uint samplerate
		);
		template <typename Task, typename View>
		void updateRamp(
				View view,
				std::function<double()> getValue,
				std::function<void(const double)> setValue,
				const PlaybackPosition position,
				const uint samplerate
		);
		std::shared_ptr<SampledFunctionCollectionImpl> getNetwork() const {
			return this->network;
		}
		void modelWorkerLoop();

	private:
		bool audioSchedulingEnabled = false;
		PlaybackPosition position = 0;
		std::atomic<uint> samplerate = 0;
		mutable std::mutex networkLock;
		std::shared_ptr<SampledFunctionCollectionImpl> network;
		mutable std::mutex tasksLock;
		std::deque<WriteTask> writeTasks;

		std::atomic<bool> stopModelWorker;
		std::thread modelWorkerThread;
		std::counting_semaphore<1> modelTasks{0};
		std::atomic<bool> expensiveTaskRunning = false;

	template <auto function>
	friend struct SetterTask;

	template <
		auto function,
		typename... Args
	>
	friend auto makeSetter(
		ScheduledFunctionCollectionImpl* obj,
		Args... args
	);

	template <auto function>
	friend void run(
		SetterTask<function>* setter
	);
};

#include "fge/model/template_utils_def.h"
