#include "fge/model/model_impl.h"
#include <cstddef>
#include <ctime>
#include <memory>
#include <mutex>
#include <strings.h>
#include <QDebug>


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

ScheduledFunctionCollectionImpl::Index ScheduledFunctionCollectionImpl::size() const
{
	START_READ()
	return getNetwork()->size();
}

void ScheduledFunctionCollectionImpl::resize( const uint size )
{
	if( !audioSchedulingEnabled ) {
		getNetwork()->resize( size );
		return;
	}
	tasksLock.lock();
	// ramp down first:
	{
		auto task = RampMasterTask{ 1, 0 };
		writeTasks.push_back(std::move(task));
	}
	// set value:
	auto future = [this,size](){
		auto task = ResizeTask{
			size
		};
		task.pos = this->position;
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	// later ramp up again:
	{
		auto task = RampMasterTask{ 0, 1 };
		writeTasks.push_back(std::move(task));
	}
	auto returnSignal = [this](){
		auto task = SignalReturnTask{};
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	tasksLock.unlock();
	future.get();
	returnSignal.get();
	return;
}

// Read entries:
QString ScheduledFunctionCollectionImpl::getFormula(
		const size_t index
) const
{
	START_READ()
	return getNetwork()->getFormula(index);
}

MaybeError ScheduledFunctionCollectionImpl::getError(
		const Index index
) const
{
	START_READ()
	return getNetwork()->getError(index);
}

// Set Entries:

MaybeError ScheduledFunctionCollectionImpl::set(
		const Index index,
		const QString& formula,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescriptions
)
{
	if( !audioSchedulingEnabled ) {
		return getNetwork()->set( index, formula, parameters, stateDescriptions );
	}
	auto functionParameters = FunctionParameters{ formula, parameters, stateDescriptions };
	tasksLock.lock();
	// ramp down first:
	for(uint i=index; i<getNetwork()->size(); i++) {
		auto task = RampTask{ i, 1, 0 };
		writeTasks.push_back(std::move(task));
	}
	// set value:
	auto future = [this,index,functionParameters](){
		auto task = SetTask{
			index, functionParameters
		};
		task.pos = this->position;
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	// later ramp up again:
	for(uint i=index; i<getNetwork()->size(); i++) {
		auto task = RampTask{ i, 0, 1 };
		writeTasks.push_back(std::move(task));
	}
	auto returnSignal = [this](){
		auto task = SignalReturnTask{};
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	tasksLock.unlock();
	auto ret = future.get();
	returnSignal.get();
	return ret;
}

MaybeError ScheduledFunctionCollectionImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	if( !audioSchedulingEnabled ) {
		return getNetwork()->setParameterValues( index,parameters );
	}
	tasksLock.lock();
	// ramp down first:
	for(uint i=index; i<getNetwork()->size(); i++) {
		auto task = RampTask{ i, 1,0 };
		writeTasks.push_back(std::move(task));
	}
	// set value:
	auto future = [this,index,parameters](){
		auto task = SetParameterValuesTask{ index,parameters };
		task.pos = this->position;
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	// later ramp up again:
	for(uint i=index; i<getNetwork()->size(); i++) {
		auto task = RampTask{ i, 0,1 };
		writeTasks.push_back(std::move(task));
	}
	auto returnSignal = [this](){
		auto task = SignalReturnTask{};
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	tasksLock.unlock();
	auto ret = future.get();
	returnSignal.get();
	return ret;
}


/***************
 * Sampling
 ***************/

// general settings:
SamplingSettings ScheduledFunctionCollectionImpl::getSamplingSettings(
		const Index index
)
{
	START_READ()
	return getNetwork()->getSamplingSettings(index);
}

void ScheduledFunctionCollectionImpl::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{
	if( !audioSchedulingEnabled ) {
		return getNetwork()->setSamplingSettings(index, value);
		return;
	}
	tasksLock.lock();
	// ramp down first:
	{
		getNetwork()->getNodeInfo(index)->volumeEnvelope = 1;
		auto task = RampTask{ index, 1,0 };
		writeTasks.push_back(std::move(task));
	}
	// set value:
	auto future = [this,index,value](){
		auto task = SetSamplingSettingsTask{
			index, value
		};
		task.pos = this->position;
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	// later ramp up again:
	{
		auto task = RampTask{ index, 0,1 };
		writeTasks.push_back(std::move(task));
		getNetwork()->getNodeInfo(index)->volumeEnvelope = 0;
	}
	auto returnSignal = [this](){
		auto task = SignalReturnTask{};
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	tasksLock.unlock();
	future.get();
	returnSignal.get();
	return;
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

void ScheduledFunctionCollectionImpl::setIsPlaybackEnabled(
		const Index index,
		const bool value
)
{
	if( !audioSchedulingEnabled ) {
		getNetwork()->setIsPlaybackEnabled( index, value );
		return;
	}
	tasksLock.lock();
	// ramp down first:
	if( !value )
	{
		getNetwork()->getNodeInfo(index)->volumeEnvelope = 1;
		auto task = RampTask{ index, 1,0 };
		writeTasks.push_back(std::move(task));
	}
	// set value:
	auto future = [this,index,value](){
		auto task = SetIsPlaybackEnabledTask{
			index, value
		};
		task.pos = this->position;
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	// later ramp up again:
	if( value )
	{
		auto task = RampTask{ index, 0,1 };
		writeTasks.push_back(std::move(task));
		getNetwork()->getNodeInfo(index)->volumeEnvelope = 0;
	}
	auto returnSignal = [this](){
		auto task = SignalReturnTask{};
		auto future = task.promise.get_future();
		writeTasks.push_back(std::move(task));
		return future;
	}();
	tasksLock.unlock();
	future.get();
	returnSignal.get();
	return;
}
void ScheduledFunctionCollectionImpl::valuesToBuffer(
		std::vector<float>* buffer,
		const PlaybackPosition position,
		const unsigned int samplerate
)
{
	START_READ()
	return getNetwork()->valuesToBuffer(buffer,position,samplerate);
}

// Control Scheduling:

void ScheduledFunctionCollectionImpl::setAudioSchedulingEnabled(
		const bool value
)
{
	// switch on:
	if( value == 1 && !audioSchedulingEnabled ) {
		audioSchedulingEnabled = value;
		tasksLock.lock();
		{
			auto task = RampMasterTask{ 0, 1 };
			writeTasks.push_back(std::move(task));
		}
		tasksLock.unlock();
		return;
	}
	// switch off:
	else if( value == 0 && audioSchedulingEnabled ) {
		// assert( masterVolumeEnv >= 0.99 );
		tasksLock.lock();
		{
			auto task = RampMasterTask{ 1, 0 };
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

void ScheduledFunctionCollectionImpl::executeWriteOperations(
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
					getNetwork()->resize( task->size );
					task->promise.set_value();
					writeTasks.pop_front();
				}
				// set:
				else if( auto task = std::get_if<SetTask>(&someTask) ) {
					auto taskRet = getNetwork()->set(
							task->index,
							task->parameters.formula,
							task->parameters.parameters,
							task->parameters.stateDescriptions
					);
					task->promise.set_value( taskRet );
					writeTasks.pop_front();
				}
				// setParameterValues:
				else if( auto task = std::get_if<SetParameterValuesTask>(&someTask) ) {
					auto taskRet = getNetwork()->setParameterValues(
							task->index,
							task->parameters
					);
					task->promise.set_value( taskRet );
					writeTasks.pop_front();
				}
				// setIsPlaybackEnabled:
				else if( auto task = std::get_if<SetIsPlaybackEnabledTask>(&someTask) ) {
					getNetwork()->getNodeInfo(task->index)->isPlaybackEnabled = task->value;
					task->promise.set_value();
					writeTasks.pop_front();
				}
				// setSamplingSettings:
				else if( auto task = std::get_if<SetSamplingSettingsTask>(&someTask) ) {
					getNetwork()->setSamplingSettings( task->index, task->value );
					task->promise.set_value();
					writeTasks.pop_front();
				}
				else if( auto task = std::get_if<SignalReturnTask>(&someTask) ) {
					task->promise.set_value();
					writeTasks.pop_front();
				}
				else if( std::get_if<RampTask>(&someTask) ) {
					if( currentEnvTaskDone ) {
						writeTasks.pop_front();
						currentEnvTaskDone = false;
					}
				}
				else if( std::get_if<RampMasterTask>(&someTask) ) {
					if( currentEnvTaskDone ) {
						writeTasks.pop_front();
						currentEnvTaskDone = false;
					}
				}
			}
			tasksLock.unlock();
		}
		networkLock.unlock();
	}
}

void ScheduledFunctionCollectionImpl::updateRamps(
		const PlaybackPosition position,
		const uint samplerate
)
{
	this->position = position;
	if( !writeTasks.empty() ) {
		auto& someTask = writeTasks.front();
		if( auto task = std::get_if<RampMasterTask>(&someTask) ) {
			if( !task->pos) {
				task->pos = position;
			}
			double time = double(position - task->pos.value()) / double(samplerate);
			if( time < rampTime ) {
				getNetwork()->setMasterVolume(
					task->src + (task->dst - task->src)
					* (time / rampTime)
				);
			}
			else {
				currentEnvTaskDone = true;
			}
		}
		else if( auto task = std::get_if<RampTask>(&someTask) ) {
			if( !task->pos) {
				task->pos = position;
			}
			double time = double(position - task->pos.value()) / double(samplerate);
			if( time < rampTime ) {
				double env = 1;
				env =
					task->src + (task->dst - task->src)
					* (time / rampTime)
				;
				getNetwork()->getNodeInfo(task->index)->volumeEnvelope = env;
			}
			else {
				currentEnvTaskDone = true;
			}
		}
	}
}
