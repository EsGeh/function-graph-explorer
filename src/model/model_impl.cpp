#include "fge/model/model_impl.h"
#include "fge/model/func_network_impl.h"
#include <ctime>
#include <mutex>
#include <strings.h>
#include <QDebug>
#include <source_location>


#ifdef LOG_MODEL

#define LOG_FUNCTION() \
{ \
	const auto location = std::source_location::current(); \
	qDebug() << location.file_name() << location.function_name(); \
}

#else
#define LOG_FUNCTION()
#endif


/************************
 * ModelImpl
************************/

#define START_READ() \
	if( audioSchedulingEnabled ) { \
		std::lock_guard lock( networkLock ); \
	}

#define LOCK_TASKS() \
	std::lock_guard lock( tasksLock );

ModelImpl::ModelImpl(
		const SamplingSettings& defSamplingSettings
):
	network( funcNetworkWithInfoFabric<NodeInfo>() )
{}

// Getters:

SamplingSettings ModelImpl::getSamplingSettings(
		const Index index
)
{
	LOG_FUNCTION()
	return no_optimization_settings;
}

uint ModelImpl::size() const
{
	LOG_FUNCTION()
	START_READ()
	return getNetwork()->size();
}

QString ModelImpl::getFormula(
		const size_t index
) const {
	LOG_FUNCTION()
	START_READ()
	return getNetwork()->getFunctionParameters(index).formula;
}

MaybeError ModelImpl::getError(
		const Index index
) const {
	LOG_FUNCTION()
	START_READ()
	auto functionOrError = getNetwork()->get(index);
	if( !functionOrError ) {
		return functionOrError.error();
	}
	return {};
}

bool ModelImpl::getIsPlaybackEnabled(
		const uint index
) const
{
	LOG_FUNCTION()
	START_READ()
	return getNetwork()->getNodeInfo(index)->isPlaybackEnabled;
}

ErrorOrValue<std::vector<std::pair<C,C>>> ModelImpl::getGraph(
		const Index index,
		const std::pair<T,T>& range,
		const unsigned int resolution
) const
{
	LOG_FUNCTION()
	START_READ()
	auto errorOrFunction = getNetwork()->get( index );
	if( !errorOrFunction ) {
		return std::unexpected( errorOrFunction.error() );
	}
	else
	{
		auto function = errorOrFunction.value();
		auto
			xMin = range.first,
			xMax = range.second
		;
		std::vector<std::pair<C,C>> graph;
		for( unsigned int i=0; i<resolution; i++ ) {
			auto x = C( xMin + (T(i) / (resolution-1))*(xMax - xMin), 0);
			graph.push_back(
					{
						x,
						function->get(x)
					}
			);
		}
		return graph;
	}
}

void ModelImpl::valuesToBuffer(
		std::vector<float>* buffer,
		const PlaybackPosition position,
		const unsigned int samplerate
)
{
	std::scoped_lock lock( networkLock, tasksLock );
	for(
			PlaybackPosition pos=0;
			pos<buffer->size();
			pos++
	) {
		buffer->data()[pos] =
			audioFunction(position+pos, samplerate);
		updateRamps(position+pos,samplerate);
	}
}

// Setters:

void ModelImpl::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{}

void ModelImpl::resize(const uint size)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->resize( size );
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

MaybeError ModelImpl::set(
		const Index index,
		const QString& formula,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescriptions
)
{
	LOG_FUNCTION()
	auto functionParameters = FunctionParameters{
		.formula = formula,
		.parameters = parameters,
		.stateDescriptions = stateDescriptions
	};
	if( !audioSchedulingEnabled ) {
		return getNetwork()->set( index, functionParameters );
	}
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

MaybeError ModelImpl::setParameterValues(
		const uint index,
		const ParameterBindings& parameters
)
{
	LOG_FUNCTION()
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

void ModelImpl::setIsPlaybackEnabled(
		const uint index,
		const bool value
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		getNetwork()->getNodeInfo(index)->isPlaybackEnabled = value;
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

// Control Scheduling:
void ModelImpl::setAudioSchedulingEnabled(
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

void ModelImpl::executeWriteOperations(
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
							task->parameters
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

// Private:

void ModelImpl::updateRamps(
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
				masterVolumeEnv =
					task->src + (task->dst - task->src)
					* (time / rampTime)
				;
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

float ModelImpl::audioFunction(
		const PlaybackPosition position,
		const uint samplerate
)
{
	double ret = 0;
	C time = C(T(position) / T(samplerate), 0);
	for( Index i=0; i<getNetwork()->size(); i++ ) {
		auto functionOrError = getNetwork()->get(i);
		auto isPlaybackEnabled = getNetwork()->getNodeInfo(i)->isPlaybackEnabled;
		if( !functionOrError || !isPlaybackEnabled )
			continue;
		auto function = functionOrError.value();
		double volEnv = getNetwork()->getNodeInfo(i)->volumeEnvelope;
		ret += (
				function->get( time ).c_.real()
				* volEnv
		);
	}
	ret *= masterVolumeEnv;
	return std::clamp( ret, -1.0, +1.0 );
}
