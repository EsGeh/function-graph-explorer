#include "fge/model/model_impl.h"
#include "include/fge/model/sampled_func_collection.h"
#include "include/fge/model/template_utils_def.h"
#include <ctime>
#include <memory>
#include <mutex>
#include <strings.h>
#include <QDebug>
#include <type_traits>
#include <ranges>


#ifndef NDEBUG
#define LOG_MODEL
#endif

#ifdef LOG_MODEL
#include <source_location>
#endif

#ifdef LOG_MODEL
#define LOG_FUNCTION() \
	{ \
		const auto location = std::source_location::current(); \
		qDebug() << QString("%1: %2").arg(double(position) / 44100.0).arg(location.function_name()); \
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
) const
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
			auto task = RampMasterEnvTask{ .dst = 1 };
			writeTasks.push_back(task);
		}
		tasksLock.unlock();
		return;
	}
	// switch off:
	else {
		// assert( masterVolumeEnv >= 0.99 );
		tasksLock.lock();
		{
			auto task = RampMasterEnvTask{ .dst = 0 };
			writeTasks.push_back(task);
		}
		auto task = SignalReturnTask{};
		auto returnSignal = task.promise.get_future();
		writeTasks.push_back(std::move(task));
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
			// 1. 
			if( !writeTasks.empty() ) {
				auto& someTask = writeTasks.front();
				std::visit( [position,samplerate](auto& task) {
						using Task = std::decay_t<decltype(task)>;
						if constexpr ( IsSetter<Task>::value ) {
							if( !task.done ) {
								run(&task);
								#ifdef LOG_MODEL
								qDebug() << QString("%1: executing '%2")
									.arg( double(position) / double(samplerate) )
									.arg( functionName(task) )
								;
								#endif
							}
						}
						else if constexpr ( std::is_same_v<Task,SignalReturnTask> ) {
							if( !task.done ) {
								task.promise.set_value();
							}
							task.done = true;
						}
				}, someTask );
			}
			#ifdef LOG_MODEL
			// Debug print ramps:
			for( auto& someTask : writeTasks ) {
				if( auto task = std::get_if<RampTask>( &someTask ); task && task->done) {
					qDebug() << QString("%1: ramp done %2: %3->%4, in %5 s")
						.arg( double(position) / double(samplerate) )
						.arg( task->index )
						.arg( task->src )
						.arg( task->dst )
						.arg( double(position - task->pos.value_or(position)) / double(samplerate) );
				}
				else if( auto task = std::get_if<RampMasterEnvTask>( &someTask ); task && task->done) {
					qDebug() << QString("%1: master env ramp done: %2->%3, in %4 s")
						.arg( double(position) / double(samplerate) )
						.arg( task->src )
						.arg( task->dst )
						.arg( double(position - task->pos.value_or(position)) / double(samplerate) );
				}
				else if( auto task = std::get_if<RampMasterVolumeTask>( &someTask ); task && task->done) {
					qDebug() << QString("%1: master volume ramp done: %2->%3, in %4 s")
						.arg( double(position) / double(samplerate) )
						.arg( task->src )
						.arg( task->dst )
						.arg( double(position - task->pos.value_or(position)) / double(samplerate) );
				}
			}
			#endif
			// cleanup ramps:
			{
				auto [rem_begin, rem_end] = std::ranges::remove_if(writeTasks, [](auto& someTask) -> bool {
					return std::visit( [](auto&& task) {
						using Task = std::decay_t<decltype(task)>;
						if constexpr ( IsSetter<Task>::value ) {
							return task.done;
						}
						else if constexpr ( std::is_same_v<Task,SignalReturnTask> ) {
							return task.done;
						}
						else if constexpr ( std::is_same_v<Task,RampTask> ) {
							return task.done;
						}
						else if constexpr ( std::is_same_v<Task,RampMasterEnvTask> ) {
							return task.done;
						}
						else if constexpr ( std::is_same_v<Task,RampMasterVolumeTask> ) {
							return task.done;
						}
						return false;
					}, someTask );
				});
				writeTasks.erase( rem_begin, rem_end );
			}
			tasksLock.unlock();
		}
		networkLock.unlock();
	}
}

// prepare impl:

void ScheduledFunctionCollectionImpl::prepareResizeImpl()
{
	auto task = RampMasterEnvTask{ .dst = 0 };
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
	prepareSetImpl(index);
}

void ScheduledFunctionCollectionImpl::prepareSetIsPlaybackEnabledImpl(const Index index, const bool value)
{
	if( value == getNetwork()->getIsPlaybackEnabled(index) ) {
		return;
	}
	if( value ) {
		updateMasterVolumeImpl();
	}
	{
		// ramp down before switching on or off:
		auto task = RampTask{ .index = index, .dst = 0 };
		writeTasks.push_back(std::move(task));
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
		if( getNetwork()->getIsPlaybackEnabled(i) ) {
			auto task = RampTask{ .index = i, .dst = 1 };
			writeTasks.push_back(std::move(task));
		}
	}
	{
		auto task = RampMasterEnvTask{ .dst = 1 };
		writeTasks.push_back(std::move(task));
	}
	updateMasterVolumeImpl();
}

void ScheduledFunctionCollectionImpl::updateMasterVolumeImpl()
{
	uint count = 0;
	{
		for(uint i=0; i<getNetwork()->size(); i++ ) {
			if( getNetwork()->getIsPlaybackEnabled(i) ){
				count++; 
			}
		}
	}
	{
		// ramp to scale master volume
		auto task = RampMasterVolumeTask{ .dst = 1.0/std::max(1.0,double(count)) };
		writeTasks.push_back(std::move(task));
	}
}

void ScheduledFunctionCollectionImpl::updateRamps(
		const PlaybackPosition position,
		const uint samplerate
)
{
	this->position = position;
	auto rampView = writeTasks
		| std::views::take_while([](auto& someTask) {
				return
					std::holds_alternative<RampTask>(someTask)
					|| std::holds_alternative<RampMasterEnvTask>(someTask)
					|| std::holds_alternative<RampMasterVolumeTask>(someTask)
				;
		})
		| std::views::reverse
	;
	// RampMasterEnvTask:
	{
		auto view = rampView
			| std::views::filter([](auto& someTask){
				auto task = std::get_if<RampMasterEnvTask>(&someTask);
				return task && !task->done;
			})
			| std::views::transform([](auto& task){
				return &std::get<RampMasterEnvTask>( task );
			})
		;
		updateRamp<RampMasterEnvTask>(
				view,
				[this]() { return getNetwork()->getMasterEnvelope(); },
				[this](const double value) { return getNetwork()->setMasterEnvelope(value); },
				position, samplerate
		);
	}
	// RampMasterVolumeTask:
	{
		auto view = rampView
			| std::views::filter([](auto& someTask){
				auto task = std::get_if<RampMasterVolumeTask>(&someTask);
				return task && !task->done;
			})
			| std::views::transform([](auto& task){
				return &std::get<RampMasterVolumeTask>( task );
			})
		;
		updateRamp<RampMasterVolumeTask>(
				view,
				[this]() { return getNetwork()->getMasterVolume(); },
				[this](const double value) { return getNetwork()->setMasterVolume(value); },
				position, samplerate
		);
	}
	// RampTask:
	for( uint index=0; index<getNetwork()->size(); index++ )
	{
		auto view = rampView
			| std::views::filter([index](auto& someTask){
				auto task = std::get_if<RampTask>(&someTask);
				return task && (task->index == index) && !task->done;
			})
			| std::views::transform([](auto& task){
				return &std::get<RampTask>( task );
			})
		;
		updateRamp<RampTask>(
				view,
				[this,index]() { return getNetwork()->getNodeInfo(index)->volumeEnvelope; },
				[this,index](const double value) { getNetwork()->getNodeInfo(index)->volumeEnvelope = value; },
				position, samplerate
		);
	}
}

template <typename Task, typename View>
void ScheduledFunctionCollectionImpl::updateRamp(
		View view,
		std::function<double()> getValue,
		std::function<void(const double)> setValue,
		const PlaybackPosition position,
		const uint samplerate
) {
	if( !view.empty() )
	{
		Task* task = view.front();
		if( !task->pos) {
			task->pos = position;
			task->src = getValue();
		}
		double time = double(position - task->pos.value()) / double(samplerate);
		double fixedRampTime = fabs(task->src - task->dst) * rampTime;
		if( time < fixedRampTime )
		{
			setValue(
				task->src + (task->dst - task->src)
				* (time / fixedRampTime)
			);
		}
		else {
			task->done = true;
		}
	}
	// ignore all previous tasks:
	for( auto* task : view | std::ranges::views::drop(1) ) {
		// initialize tasks if necessary
		// for correct debug output:
		if( !task->pos) {
			task->pos = position;
			task->src = getValue();
		}
		task->done = true;
	}
}
