#pragma once

#include "fge/model/function.h"
#include "fge/shared/data.h"
#include <memory>
#include <future>


struct FunctionParameters
{
	QString formula;
	ParameterBindings parameters;
	StateDescriptions stateDescriptions;
};

using FunctionOrError =
	std::expected<std::shared_ptr<Function>,Error>;

class FuncNetwork
{
	public:
		using Index = uint;
	public:
		// Size
		virtual uint size() const = 0;
		virtual void resize( const uint size ) = 0;

		// Read / write entries:
		virtual FunctionOrError get(const uint index) const = 0;

		virtual MaybeError set(
				const Index index,
				const FunctionParameters& parameters
		) = 0;
		virtual FunctionParameters getFunctionParameters(const uint index) const = 0;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) = 0;

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const = 0;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) = 0;
};

struct InvalidEntry
{
	QString error;
	FunctionParameters parameters;
};

using FunctionOrInvalid = std::expected<
	std::shared_ptr<Function>,
	InvalidEntry
>;


struct NetworkEntry
{
	FunctionOrInvalid functionOrError;
	bool isAudioEnabled = false;
};

class FuncNetworkImpl:
	public FuncNetwork
{
	public:
		FuncNetworkImpl();

		virtual uint size() const override;
		virtual void resize( const uint size ) override;

		virtual std::expected<std::shared_ptr<Function>,Error> get(const Index index) const override;
		virtual MaybeError set(
				const Index index,
				const FunctionParameters& parameters
		) override;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;

		virtual FunctionParameters getFunctionParameters(const uint index) const override;

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;

	private:
		void updateFormulas(
				const size_t startIndex,
				const std::optional<FunctionParameters>& parameters
		);
	private:
		Symbols constants;
		std::vector<std::shared_ptr<NetworkEntry>> entries;
};

class FuncNetworkHighLevel {
	public:
		using Index = FuncNetwork::Index;
		using PlaybackPosition = unsigned long int;
	public:
		virtual float audioFunction(
				const PlaybackPosition position,
				const uint samplerate
		) = 0;
		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) = 0;
};

const SamplingSettings no_optimization_settings{
	.resolution = 0,
	.interpolation = 0,
	.caching = false
};

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

class FuncNetworkScheduled:
	public FuncNetwork,
	public FuncNetworkHighLevel
{
	public:
		using FuncNetwork::Index;
	public:
		FuncNetworkScheduled(
				const SamplingSettings& defSamplingSettings = no_optimization_settings
		);
		SamplingSettings getSamplingSettings(
				const Index index
		);
		void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		);
		virtual Index size() const override;
		virtual void resize( const uint size ) override;

		virtual FunctionOrError get(const Index index) const override;
		virtual MaybeError set(
				const Index index,
				const FunctionParameters& parameters
		) override;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;

		virtual FunctionParameters getFunctionParameters(const uint index) const override;

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;

		std::mutex& readLock() {
			return networkLock;
		}
		virtual float audioFunction(
				const PlaybackPosition position,
				const uint samplerate
		) override;

		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) override;

		void updateRamps(
				const PlaybackPosition position,
				const uint samplerate
		);
		void executeWriteOperations(
				const PlaybackPosition position,
				const uint samplerate
		);

		// drive the scheduler:
		void setAudioSchedulingEnabled(
				const bool value
		);

	private:
		FuncNetworkImpl* getNetwork() const {
			return this->network.get();
		}
		double currentEnvVal(const Index index) const;

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
				decltype(std::declval<FuncNetworkScheduled>().setIsPlaybackEnabled(index,value));
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
		mutable std::mutex networkLock;
		std::shared_ptr<FuncNetworkImpl> network = std::make_shared<FuncNetworkImpl>();
		mutable std::mutex tasksLock;
		std::deque<WriteTask> writeTasks;
};

class Model:
	protected FuncNetworkScheduled
{
	public:
		using Father = FuncNetworkScheduled;
	public:
		Model(
				const SamplingSettings& defSamplingSettings = no_optimization_settings
		)
			: FuncNetworkScheduled( defSamplingSettings )
		{}
		using Father::size;
		using Father::resize;

		MaybeError set(
				const Index index,
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescriptions
		)
		{
			return Father::set(
					index,
					FunctionParameters{
						.formula = formula,
						.parameters = parameters,
						.stateDescriptions = stateDescriptions
					}
			);
		}

		using Father::setParameterValues;
		using Father::getSamplingSettings;
		using Father::setSamplingSettings;
		using Father::getGraph;
		using Father::audioFunction;
		using Father::getIsPlaybackEnabled;
		using Father::setIsPlaybackEnabled;
		using Father::setAudioSchedulingEnabled;
		using Father::updateRamps;
		using Father::executeWriteOperations;
		using Father::readLock;

		QString getFormula(
				const size_t index
		) {
			return Father::getFunctionParameters(index).formula;
		}
		MaybeError getError(
				const Index index
		) {
			auto functionOrError = Father::get(index);
			if( !functionOrError ) {
				return functionOrError.error();
			}
			return {};
		}
};
