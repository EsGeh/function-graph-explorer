#include "fge/model/model_impl.h"
#include "include/fge/model/sampled_func_collection.h"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <mutex>
#include <strings.h>
#include <QDebug>
#include <ranges>


// #define LOG_MODEL

#ifdef LOG_MODEL
#include <source_location>
#endif

#ifdef LOG_MODEL
#define LOG_FUNCTION() \
	{ \
		const auto location = std::source_location::current(); \
		qDebug() << location.function_name(); \
	}
#else
#define LOG_FUNCTION()
#endif

/************************
 * ScheduledFunctionCollectionImpl:
************************/

#define START_READ() \
	std::lock_guard lock( networkLock );

ScheduledFunctionCollectionImpl::ScheduledFunctionCollectionImpl(
		const SamplingSettings& defSamplingSettings
)
	: network( new SampledFunctionCollectionImpl(
				defSamplingSettings,
				[this](
					const PlaybackPosition position,
					const uint samplerate
				) {
					updateRamps(position, samplerate);
				}
	))
{
}

/************************
 * READ:
************************/

ScheduledFunctionCollectionImpl::Index ScheduledFunctionCollectionImpl::size() const
{
	START_READ()
	return getNetwork()->size();
}

// Read entries:
FunctionParameters ScheduledFunctionCollectionImpl::get(
		const size_t index
) const
{
	START_READ()
	return getNetwork()->get(index);
}

MaybeError ScheduledFunctionCollectionImpl::getError(
		const Index index
) const
{
	START_READ()
	return getNetwork()->getError(index);
}

SamplingSettings ScheduledFunctionCollectionImpl::getSamplingSettings(
		const Index index
)
{
	START_READ()
	return getNetwork()->getSamplingSettings(index);
}

// sampling for visual representation:
ErrorOrValue<std::vector<std::pair<C,C>>> ScheduledFunctionCollectionImpl::getGraph(
		const Index index,
		const std::pair<T,T>& range,
		const unsigned int resolution
) const
{
	START_READ()
	return getNetwork()->getGraph(index, range, resolution);
}

// sampling for audio:
bool ScheduledFunctionCollectionImpl::getIsPlaybackEnabled(
		const Index index
) const
{
	START_READ()
	return getNetwork()->getIsPlaybackEnabled(index);
}

/************************
 * WRITE:
************************/

void ScheduledFunctionCollectionImpl::prepareResize()
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	tasksLock.lock();
	prepareResizeImpl();
	tasksLock.unlock();
}

void ScheduledFunctionCollectionImpl::prepareSet(const Index index)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	tasksLock.lock();
	prepareSetImpl(index);
	tasksLock.unlock();
}

void ScheduledFunctionCollectionImpl::prepareSetParameterValues(const Index index)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	tasksLock.lock();
	prepareSetParameterValuesImpl(index);
	tasksLock.unlock();
}

void ScheduledFunctionCollectionImpl::prepareSetIsPlaybackEnabled(const Index index, const bool value)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	tasksLock.lock();
	prepareSetIsPlaybackEnabledImpl(index, value);
	tasksLock.unlock();
}

void ScheduledFunctionCollectionImpl::prepareSetSamplingSettings(const Index index)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	tasksLock.lock();
	prepareSetSamplingSettingsImpl(index);
	tasksLock.unlock();
}

// post:

void ScheduledFunctionCollectionImpl::postSetAny()
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	tasksLock.lock();
	postSetAnyImpl();
	tasksLock.unlock();
}

void ScheduledFunctionCollectionImpl::resize( const uint size )
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		getNetwork()->resize( size );
		return;
	}
	tasksLock.lock();
	// ramp down first:
	prepareResizeImpl();
	// set value:
	auto future = makeSetter<::resize>(
			this,
			size
	);
	tasksLock.unlock();
	future.get();
	return;
}

MaybeError ScheduledFunctionCollectionImpl::bulkUpdate(
		const Index index,
		const Update& update
)
{
	LOG_FUNCTION()
	// prepare:
	{
		if(
				update.formula.has_value()
				|| update.parameters.has_value()
				|| update.stateDescriptions.has_value()
		) {
			prepareSet(index);
		}
		if( update.playbackEnabled.has_value() ) {
			prepareSetIsPlaybackEnabled(index, update.playbackEnabled.value());
		}
		if( update.samplingSettings.has_value() ) {
			prepareSetSamplingSettings(index);
		}
	}
	// set:
	auto maybeError = [this,index,update](){
		MaybeError ret{};
		if(
				update.formula.has_value()
				|| update.parameters.has_value()
				|| update.stateDescriptions.has_value()
		) {
			ret = set(
					index,
					update.formula.value_or( get(index).formula ),
					update.parameters.value_or( get(index).parameters ),
					update.stateDescriptions.value_or( get(index).stateDescriptions )
			);
		}
		if( update.playbackEnabled.has_value() ) {
			setIsPlaybackEnabled(index, update.playbackEnabled.value());
		}
		if( update.samplingSettings.has_value() ) {
			setSamplingSettings(index, update.samplingSettings.value());
		}
		return ret;
	}();
	// turn on audio, if muted:
	postSetAny();
	return maybeError;
}

// Set Entries:

MaybeError ScheduledFunctionCollectionImpl::set(
		const Index index,
		const QString& formula,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescriptions
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->set( index, formula, parameters, stateDescriptions );
	}
	auto functionParameters = FunctionParameters{ formula, parameters, stateDescriptions };
	tasksLock.lock();
	// ramp down first:
	prepareSetImpl(index);
	// set value:
	auto future = makeSetter<::set>(
				this,
				index, formula, parameters, stateDescriptions
	);
	tasksLock.unlock();
	auto ret = future.get();
	return ret;
}

MaybeError ScheduledFunctionCollectionImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->setParameterValues( index,parameters );
	}
	tasksLock.lock();
	// ramp down first:
	prepareSetParameterValuesImpl(index);
	// set value:
	auto future = makeSetter<::setParameterValues>(
				this,
				index, parameters
	);
	tasksLock.unlock();
	auto ret = future.get();
	return ret;
}

void ScheduledFunctionCollectionImpl::setIsPlaybackEnabled(
		const Index index,
		const bool value
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		getNetwork()->setIsPlaybackEnabled( index, value );
		return;
	}
	tasksLock.lock();
	// ramp down first:
	prepareSetIsPlaybackEnabledImpl(index, value);
	// set value:
	auto future = makeSetter<::setIsPlaybackEnabled>(
			this,
			index,value
	);
	tasksLock.unlock();
	future.get();
	return;
}

void ScheduledFunctionCollectionImpl::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->setSamplingSettings(index, value);
		return;
	}
	tasksLock.lock();
	// ramp down first:
	prepareSetSamplingSettingsImpl(index);
	// set value:
	auto future = makeSetter<::setSamplingSettings>(
			this,
			index, value
	);
	tasksLock.unlock();
	future.get();
	return;
}

// Control Scheduling:

void ScheduledFunctionCollectionImpl::valuesToBuffer(
		std::vector<float>* buffer,
		const PlaybackPosition position,
		const unsigned int samplerate
)
{
	START_READ()
	return getNetwork()->valuesToBuffer(buffer,position,samplerate);
}

void ScheduledFunctionCollectionImpl::setAudioSchedulingEnabled(
		const bool value
)
{
	LOG_FUNCTION()
	if( value == audioSchedulingEnabled ) {
		return;
	}
	// switch on:
	if( value == true ) {
		audioSchedulingEnabled = value;
		tasksLock.lock();
		{
			auto task = RampMasterTask{ .dst = 1 };
			writeTasks.push_back(std::move(task));
		}
		tasksLock.unlock();
		return;
	}
	// switch off:
	else {
		// assert( masterVolumeEnv >= 0.99 );
		tasksLock.lock();
		{
			auto task = RampMasterTask{ .dst = 0 };
			writeTasks.push_back(std::move(task));
		}
		auto returnSignal = [this](){
			auto task = SignalReturnTask{};
			auto future = task.promise.get_future();
			writeTasks.push_back(std::move(task));
			return future;
		}();
		tasksLock.unlock();
		returnSignal.get();
		// assert( masterVolumeEnv < 0.01 );
		audioSchedulingEnabled = value;
		return;
	}
}

void ScheduledFunctionCollectionImpl::betweenAudio(
		const PlaybackPosition position,
		const uint samplerate
)
{
	this->position = position;
	if( networkLock.try_lock() ) {
		if( tasksLock.try_lock() ) {
			if( !writeTasks.empty() ) {
				auto& someTask = writeTasks.front();
				// resize:
				if( auto task = std::get_if<ResizeTask>(&someTask) ) {
					run(task);
					qDebug() << "executing 'resize'";
				}
				// set:
				else if( auto task = std::get_if<SetTask>(&someTask) ) {
					run(task);
					qDebug() << "executing 'set'";
				}
				// setParameterValues:
				else if( auto task = std::get_if<SetParameterValuesTask>(&someTask) ) {
					run(task);
					qDebug() << "executing 'setParameterValues'";
				}
				// setIsPlaybackEnabled:
				else if( auto task = std::get_if<SetIsPlaybackEnabledTask>(&someTask) ) {
					run(task);
					qDebug() << "executing 'setIsPlaybackEnabled'";
				}
				// setSamplingSettings:
				else if( auto task = std::get_if<SetSamplingSettingsTask>(&someTask) ) {
					run(task);
					qDebug() << "executing 'setSamplingSettings'";
				}
				else if( auto task = std::get_if<SignalReturnTask>(&someTask) ) {
					task->promise.set_value();
					writeTasks.pop_front();
				}
				else if( auto task = std::get_if<RampTask>(&someTask) ) {
					if( task->done ) {
						qDebug() << QString("ramp done %1: %2->%3, in %4 s")
							.arg( task->index )
							.arg( task->src )
							.arg( task->dst )
							.arg( double(position - task->pos.value_or(position)) / double(samplerate) );
						writeTasks.pop_front();
					}
				}
				else if( auto task = std::get_if<RampMasterTask>(&someTask) ) {
					if( task->done ) {
						qDebug() << QString("master ramp done: %1->%2, in %3 s")
							.arg( task->src )
							.arg( task->dst )
							.arg( double(position - task->pos.value_or(position)) / double(samplerate) );
						writeTasks.pop_front();
					}
				}
			}
			tasksLock.unlock();
		}
		networkLock.unlock();
	}
}

// prepare impl:

void ScheduledFunctionCollectionImpl::prepareResizeImpl()
{
	auto task = RampMasterTask{ .dst = 0 };
	writeTasks.push_back(std::move(task));
}

void ScheduledFunctionCollectionImpl::prepareSetImpl(const Index index)
{
	// ramp down first:
	for(uint i=index; i<getNetwork()->size(); i++) {
		auto task = RampTask{ .index = i, .dst = 0 };
		writeTasks.push_back(std::move(task));
	}
}

void ScheduledFunctionCollectionImpl::prepareSetParameterValuesImpl(const Index index)
{
	return prepareSet(index);
}

void ScheduledFunctionCollectionImpl::prepareSetIsPlaybackEnabledImpl(const Index index, const bool value)
{
	if( value == getNetwork()->getIsPlaybackEnabled(index) ) {
		return;
	}
	if( !value )
	{
		// ramp down before switching off:
		auto task = RampTask{ .index = index, .dst = 0 };
		writeTasks.push_back(std::move(task));
	}
	else {
		// silence before switching on:
		// (ramp up afterwards)
		getNetwork()->getNodeInfo(index)->volumeEnvelope = 0;
	}
}

void ScheduledFunctionCollectionImpl::prepareSetSamplingSettingsImpl(const Index index)
{
	auto task = RampTask{ .index = index, .dst = 0 };
	writeTasks.push_back(std::move(task));
}

// post set impl:

void ScheduledFunctionCollectionImpl::postSetAnyImpl()
{
	for(uint i=0; i<getNetwork()->size(); i++) {
		auto task = RampTask{ .index = i, .dst = 1 };
		writeTasks.push_back(std::move(task));
	}
	auto task = RampMasterTask{ .dst = 1 };
	writeTasks.push_back(std::move(task));
}

void ScheduledFunctionCollectionImpl::updateRamps(
		const PlaybackPosition position,
		const uint samplerate
)
{
	this->position = position;
	auto rampView = writeTasks
		| std::views::take_while([](auto& someTask) {
				return std::holds_alternative<RampMasterTask>(someTask)
					|| std::holds_alternative<RampTask>(someTask);
		})
		| std::views::reverse
	;
	// Master:
	{
		auto masterView = rampView
			| std::views::filter([](auto& someTask){
				auto task = std::get_if<RampMasterTask>(&someTask);
				return task && !task->done;
			})
			| std::views::transform([](auto& task){
				return std::get_if<RampMasterTask>( &task );
			})
		;
		if( !masterView.empty() )
		{
			RampMasterTask* task = masterView.front();
			if( !task->pos) {
				task->pos = position;
				task->src = getNetwork()->getMasterVolume();
			}
			double time = double(position - task->pos.value()) / double(samplerate);
			double fixedRampTime = abs(task->src - task->dst) * rampTime;
			if( time < fixedRampTime )
			{
				getNetwork()->setMasterVolume(
					task->src + (task->dst - task->src)
					* (time / fixedRampTime)
				);
			}
			else {
				task->done = true;
			}
		}
		// ignore all previous tasks:
		for( auto* task : masterView | std::ranges::views::drop(1) ) {
			task->done = true;
		}
	}
	// ramps by function:
	for( uint index=0; index<getNetwork()->size(); index++ )
	{
		auto view = rampView
			| std::views::filter([index](auto& someTask){
				auto task = std::get_if<RampTask>(&someTask);
				return task && task->index == index && !task->done;
			})
			| std::views::transform([](auto& task){
				return std::get_if<RampTask>( &task );
			})
		;
		if( !view.empty() )
		{
			auto* task = view.front();
			// initialize ramp task,
			// if seen for the first time:
			if( !task->pos) {
				task->pos = position;
				task->src = getNetwork()->getNodeInfo(task->index)->volumeEnvelope;
			}
			double time = double(position - task->pos.value()) / double(samplerate);
			double fixedRampTime = abs(task->src - task->dst) * rampTime;
			if( time < fixedRampTime ) {
				getNetwork()->getNodeInfo(task->index)->volumeEnvelope = 
					task->src + (task->dst - task->src)
					* (time / fixedRampTime)
				;
			}
			else {
				task->done = true;
			}
		}
		// ignore all previous tasks:
		for( auto* task : view | std::ranges::views::drop(1) ) {
			task->done = true;
		}
	}
}
