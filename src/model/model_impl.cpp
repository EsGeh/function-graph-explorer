#include "fge/model/model_impl.h"
#include "include/fge/model/model.h"
#include <cstddef>
#include <ctime>
#include <memory>
#include <mutex>
#include <strings.h>
#include <QDebug>

// #define LOG_MODEL

#ifdef LOG_MODEL
#include <source_location>
#endif


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
 * SampledFunctionCollectionImpl
************************/

SampledFunctionCollectionImpl::SampledFunctionCollectionImpl(
		const SamplingSettings& defSamplingSettings,
		AudioCallback audioCallback 
)
	: defSamplingSettings( defSamplingSettings )
	, audioCallback( audioCallback )
{}

// Read entries:
QString SampledFunctionCollectionImpl::getFormula(
		const size_t index
) const
{
	LOG_FUNCTION()
	return LowLevel::getFunctionParameters(index).formula;
}

MaybeError SampledFunctionCollectionImpl::getError(
		const Index index
) const
{
	LOG_FUNCTION()
	auto functionOrError = LowLevel::get(index);
	if( !functionOrError ) {
		return functionOrError.error();
	}
	return {};
}

// Set Entries:

MaybeError SampledFunctionCollectionImpl::set(
		const Index index,
		const QString& formula,
		const ParameterBindings& parameters,
		const StateDescriptions& stateDescriptions
)
{
	LOG_FUNCTION()
	auto ret = LowLevel::set( index, 
		FunctionParameters{
			.formula = formula,
			.parameters = parameters,
			.stateDescriptions = stateDescriptions
		}
	);
	updateCaches(index);
	return ret;
}

MaybeError SampledFunctionCollectionImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	LOG_FUNCTION()
	updateCaches(index);
	return LowLevel::setParameterValues( index,parameters );
}


/***************
 * Sampling
 ***************/

// general settings:
SamplingSettings SampledFunctionCollectionImpl::getSamplingSettings(
		const Index index
)
{
	return getNodeInfo(index)->samplingSettings;
}

void SampledFunctionCollectionImpl::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{
	LOG_FUNCTION()
	getNodeInfo(index)->samplingSettings = value;
	if( value.caching ) {
		std::shared_ptr<Function> maybeFunction = nullptr;
		auto functionOrError = get(index);
		if( functionOrError ) {
			maybeFunction = functionOrError.value();
		}
		getNodeInfo(index)->cache = updateCache(maybeFunction);
	}
	else {
		getNodeInfo(index)->cache = nullptr;
	}
}

// sampling for visual representation:
ErrorOrValue<std::vector<std::pair<C,C>>> SampledFunctionCollectionImpl::getGraph(
		const Index index,
		const std::pair<T,T>& range,
		const unsigned int resolution
) const
{
	LOG_FUNCTION()
	auto errorOrFunction = LowLevel::get( index );
	if( !errorOrFunction ) {
		return std::unexpected( errorOrFunction.error() );
	}
	else
	{
		auto func = errorOrFunction.value();
		auto nodeInfo = getNodeInfoConst(index);
		auto function = [this,func,nodeInfo](auto x) {
			return getWithResolution(
					func, x,
					nodeInfo->samplingSettings,
					nodeInfo->cache
			);
		};
		auto
			xMin = range.first,
			xMax = range.second
		;
		std::vector<std::pair<C,C>> graph;
		for( unsigned int i=0; i<resolution; i++ ) {
			auto x = C( xMin + (T(i) / (resolution-1))*(xMax - xMin), 0);
			graph.push_back({
					x,
					function(x)
			});
		}
		return graph;
	}
}

// sampling for audio:
bool SampledFunctionCollectionImpl::getIsPlaybackEnabled(
		const Index index
) const
{
	LOG_FUNCTION()
	return getNodeInfoConst(index)->isPlaybackEnabled;
}

void SampledFunctionCollectionImpl::setIsPlaybackEnabled(
		const Index index,
		const bool value
)
{
	LOG_FUNCTION()
	getNodeInfo(index)->isPlaybackEnabled = value;
}

void SampledFunctionCollectionImpl::valuesToBuffer(
		std::vector<float>* buffer,
		const PlaybackPosition position,
		const unsigned int samplerate
)
{
	for(
			PlaybackPosition pos=0;
			pos<buffer->size();
			pos++
	) {
		buffer->data()[pos] =
			audioFunction(position+pos, samplerate);
		audioCallback( position+pos, samplerate );
	}
}

double SampledFunctionCollectionImpl::getMasterVolume() const 
{
	return masterVolume;
}

void SampledFunctionCollectionImpl::setMasterVolume(const double value)
{
	masterVolume = value;
}

// private:

float SampledFunctionCollectionImpl::audioFunction(
		const PlaybackPosition position,
		const uint samplerate
)
{
	double ret = 0;
	C time = C(T(position) / T(samplerate), 0);
	for( Index i=0; i<size(); i++ ) {
		auto functionOrError = LowLevel::get(i);
		auto isPlaybackEnabled = getNodeInfo(i)->isPlaybackEnabled;
		if( !functionOrError || !isPlaybackEnabled )
			continue;
		auto function = functionOrError.value();
		double volEnv = getNodeInfo(i)->volumeEnvelope;
		ret += (
				function->get( time ).c_.real()
				* volEnv
		);
	}
	ret *= masterVolume;
	return std::clamp( ret, -1.0, +1.0 );
}

std::shared_ptr<SampledFunctionCollectionImpl::LowLevel::NodeInfo> SampledFunctionCollectionImpl::createNodeInfo(
		const Index index,
		std::shared_ptr<Function> maybeFunction
)
{
	LOG_FUNCTION()
	std::shared_ptr<Cache> cache = updateCache(maybeFunction);
	return std::shared_ptr<NodeInfo>(new ::NodeInfo{
			.samplingSettings = defSamplingSettings,
			.cache = cache
	});
};

void SampledFunctionCollectionImpl::updateCaches( const Index index )
{
	for(uint i=index; i<size(); i++) {
		if( getSamplingSettings(i).caching ) {
			std::shared_ptr<Function> maybeFunction = nullptr;
			auto functionOrError = get(i);
			if( functionOrError ) {
				maybeFunction = functionOrError.value();
			}
			getNodeInfo(i)->cache = updateCache(maybeFunction);
		}
	}
}

std::shared_ptr<Cache> SampledFunctionCollectionImpl::updateCache(
		std::shared_ptr<Function> maybeFunction
)
{
	if( maybeFunction ) {
		return std::make_shared<Cache>( [this,maybeFunction](int x) {
			return rasterIndexToY(maybeFunction, x, defSamplingSettings.resolution);
		} );
	}
	return nullptr;
}

C getWithResolution(
		std::shared_ptr<Function> function,
		const C& x,
		const SamplingSettings& samplingSettings,
		std::shared_ptr<Cache> cache
)
{
	if(
			samplingSettings.resolution == 0
			|| x.c_.imag() != 0
	) {
		return function->get(x);
	}
	/* consider `interpolation+1` points,
	 * 	centered around `x`
	 *   x(0-shift) ... x(k+1-shift)
	 *
	 * eg. with interpolation == 3:
	 *
	 *   shift == -1.
	 *   x(-1), x(0), x(1), x(2)
	 * where
	 * 		x is between x(0) and x(1)
	 * 	
	 * */
	const int shift =
		(samplingSettings.interpolation != 0)
		? (samplingSettings.interpolation+1-2)/2
		: 0;
	std::vector<C> ys;
	for( int i{0}; i<int(samplingSettings.interpolation+1); i++ ) {
		const int xpos = xToRasterIndex(x, samplingSettings.resolution) + i - shift;
		if( samplingSettings.caching ) {
			assert( cache );
			ys.push_back(cache->lookup( xpos ).first);
		}
		else {
			ys.push_back(function->get(
					C(
						T(xpos) / samplingSettings.resolution,
						0
					)
			));
		}
	};
	return interpolate( x, ys, shift, samplingSettings.resolution);
}

const T epsilon = 1.0/(1<<20);

C interpolate(
		const C& x,
		const std::vector<C> ys,
		const int shift,
		const uint resolution

)
{
	T i_temp;
	T xfrac = std::modf( x.c_.real() * resolution + epsilon, &i_temp);
	if( x.c_.real() < 0 ) {
		xfrac *= -1;
		xfrac = 1-xfrac;
	}

	C sum = C(0,0);
	for(int i=0; i<int(ys.size()); i++) {
		T factor = 1;
		for(int j=0; j<int(ys.size()); j++) {
			if( j == i ) { continue; }
			factor *= (xfrac - T(j-shift) );
			factor /= T(i - j);
		}
		sum += ys[i] * C(factor, 0);
	}
	return sum;
}

int xToRasterIndex(
		const C& x,
		const uint resolution
)
{
		return std::floor(x.c_.real() * resolution + epsilon);
}

C rasterIndexToY(
		std::shared_ptr<Function> function,
		int x,
		const uint resolution
)
{
	return function->get(
			C(T(x) / resolution,0)
	);
}

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
