#include "fge/model/model_impl.h"
#include "include/fge/model/sampled_func_collection.h"
#include "include/fge/model/sampled_func_collection_impl.h"
#include "include/fge/model/template_utils_def.h"
#include <cstring>
#include <ctime>
#include <memory>
#include <strings.h>
#include <QDebug>
#include <ranges>
#include <algorithm>


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
 * Utils:
************************/

template <auto function, typename... Args>
struct WritePrepare;

template <auto function>
struct WritePrepare<function>
{
	// static_assert( IsSetter<function>::value );
	template <typename TaskQueue>
	static void prepare(
			TaskQueue& tasksQueue
	)
	{
		Ramping::rampMasterEnv( tasksQueue, 0 );
	}
};

template <>
struct WritePrepare<&ScheduledFunctionCollectionImpl::setIsPlaybackEnabled>
{
	template <typename TaskQueue>
	static void prepare(
			TaskQueue& tasksQueue,
			const std::shared_ptr<SampledFunctionCollectionImpl> network,
			const uint index, const bool value
	)
	{
		if( value ) {
			Ramping::adjustMasterVolume(tasksQueue, network);
		}
		Ramping::rampMasterEnv( tasksQueue, 0 );
	}
};

template <typename TaskQueue>
static void postWrite(
		TaskQueue& tasksQueue,
		const std::shared_ptr<SampledFunctionCollectionImpl> network
) {
	for(uint index=0; index<network->size(); index++) {
		if( network->getIsPlaybackEnabled(index) ) {
			Ramping::rampEntry(tasksQueue, index, 1);
		}
	}
	Ramping::rampMasterEnv( tasksQueue, 1 );
	Ramping::adjustMasterVolume(tasksQueue, network);
};

/************************
 * ScheduledFunctionCollectionImpl:
************************/

ScheduledFunctionCollectionImpl::ScheduledFunctionCollectionImpl(
		const SamplingSettings& defSamplingSettings
)
	: guardedNetwork( std::make_shared<SampledFunctionCollectionImpl>(
				defSamplingSettings
	))
	, modelWorkerThread([this](){ modelWorkerLoop(); } )
{
}

ScheduledFunctionCollectionImpl::~ScheduledFunctionCollectionImpl()
{
	stopModelWorker = true;
	modelTasks.release();
	modelWorkerThread.join();
}

/************************
 * READ:
************************/

ScheduledFunctionCollectionImpl::Index ScheduledFunctionCollectionImpl::size() const
{
	return getNetworkConst()->read([](auto& network){ return network->size(); });
}

// Read entries:
FunctionParameters ScheduledFunctionCollectionImpl::get(
		const size_t index
) const
{
	return getNetworkConst()->read([index](auto& network) {
			return network->get(index);
	});
}

MaybeError ScheduledFunctionCollectionImpl::getError(
		const Index index
) const
{
	return getNetworkConst()->read([index](auto& network){
			return network->getError(index);
	});
}

SamplingSettings ScheduledFunctionCollectionImpl::getSamplingSettings(
		const Index index
) const
{
	return getNetworkConst()->read([index](auto& network){
			return network->getSamplingSettings(index);
	});
}

// sampling for visual representation:
ErrorOrValue<std::vector<std::pair<C,C>>> ScheduledFunctionCollectionImpl::getGraph(
		const Index index,
		const std::pair<T,T>& range,
		const unsigned int resolution
) const
{
	return getNetworkConst()->read([index,range,resolution](auto& network){
			return network->getGraph(index, range, resolution);
	});
}

// sampling for audio:
bool ScheduledFunctionCollectionImpl::getIsPlaybackEnabled(
		const Index index
) const
{
	return getNetworkConst()->read([index](auto& network){
			return network->getIsPlaybackEnabled(index);
	});
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
	writeTasks.write([](auto& tasksQueue) {
			WritePrepare<&ScheduledFunctionCollectionImpl::resize>::prepare(tasksQueue);
	});
}

void ScheduledFunctionCollectionImpl::prepareSet(const Index index)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	// ramp down first:
	writeTasks.write([](auto& tasksQueue) {
			WritePrepare<&ScheduledFunctionCollectionImpl::set>::prepare(tasksQueue);
	});
}

void ScheduledFunctionCollectionImpl::prepareSetParameterValues(const Index index)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	writeTasks.write([](auto& tasksQueue) {
			WritePrepare<&ScheduledFunctionCollectionImpl::setParameterValues>::prepare(tasksQueue);
	});
}

void ScheduledFunctionCollectionImpl::prepareSetIsPlaybackEnabled(const Index index, const bool value)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	writeTasks.write([this,index,value](auto& tasksQueue) {
	getNetwork()->read([&tasksQueue,index,value](auto& network) {
			WritePrepare<&ScheduledFunctionCollectionImpl::setIsPlaybackEnabled>::prepare(
					tasksQueue,
					network,
					index, value
			);
	});
	});
}

void ScheduledFunctionCollectionImpl::prepareSetSamplingSettings(const Index index)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	writeTasks.write([](auto& tasksQueue) {
			WritePrepare<&ScheduledFunctionCollectionImpl::setSamplingSettings>::prepare(
					tasksQueue
			);
	});
}

// post:

void ScheduledFunctionCollectionImpl::postSetAny()
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return;
	}
	writeTasks.write([this](auto& tasksQueue) {
	getNetwork()->read([&tasksQueue](auto& network) {
			postWrite(
					tasksQueue,
					network
			);
	});
	});
}

void ScheduledFunctionCollectionImpl::resize( const uint size )
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		getNetwork()->write([size](auto& network){
			network->resize( size );
		});
		return;
	}
	auto future = writeTasks.write([&](auto& tasksQueue) {
		WritePrepare<&ScheduledFunctionCollectionImpl::resize>::prepare(tasksQueue);
		// schedule model change:
		return makeSetter<::resize>(
				tasksQueue,
				this->position,
				size
		);
	});
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
		return getNetwork()->write([index,formula,parameters,stateDescriptions](auto& network) {
				return network->set( index, formula, parameters, stateDescriptions );
		});
	}
	auto functionParameters = FunctionParameters{ formula, parameters, stateDescriptions };
	auto future = writeTasks.write([&](auto& tasksQueue) {
		// ramp down first:
		WritePrepare<&ScheduledFunctionCollectionImpl::set>::prepare(tasksQueue);
		// schedule model change:
		return makeSetter<::set>(
					tasksQueue,
					this->position,
					index, formula, parameters, stateDescriptions
		);
	});
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
		return getNetwork()->write([index,&parameters](auto& network) {
				return network->setParameterValues( index, parameters );
		});
	}
	auto future = writeTasks.write([&](auto& tasksQueue) {
		// ramp down first:
		WritePrepare<&ScheduledFunctionCollectionImpl::setParameterValues>::prepare(tasksQueue);
		// schedule model change:
		return makeSetter<::setParameterValues>(
				tasksQueue,
				this->position,
				index, parameters
		);
	});
	auto ret = future.get();
	return ret;
}

void ScheduledFunctionCollectionImpl::scheduleSetParameterValues(
		const Index index,
		const ParameterBindings& parameters,
		ParameterSignalDone signalizeDone
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		auto ret = getNetwork()->write([index,&parameters](auto& network) {
				return network->setParameterValues( index, parameters );
		});
		signalizeDone(
				index,
				parameters
		);
	}
	writeTasks.write([&](auto& tasksQueue) {
		for( auto [parameterName, value] : parameters ) {
			Ramping::rampParameter(
				tasksQueue,
				index,
				parameterName,
				value.at(0).c_.real(),
				signalizeDone
			);
		}
	});
}

void ScheduledFunctionCollectionImpl::setIsPlaybackEnabled(
		const Index index,
		const bool value
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->write([index,value](auto& network) {
			return network->setIsPlaybackEnabled( index, value );
		});
		return;
	}
	auto future = writeTasks.write([&](auto& tasksQueue) {
		// ramp down first:
		getNetwork()->read([&](auto& network) {
			WritePrepare<&ScheduledFunctionCollectionImpl::setIsPlaybackEnabled>::prepare(
					tasksQueue,
					network,
					index, value
			);
		});
		// schedule model change:
		return makeSetter<::setIsPlaybackEnabled>(
				tasksQueue,
				this->position,
				index,value
		);
	});
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
		return getNetwork()->write([index,value](auto& network) {
			return network->setSamplingSettings(index, value);
		});
	}
	auto future = writeTasks.write([&](auto& tasksQueue) {
		// ramp down first:
		WritePrepare<&ScheduledFunctionCollectionImpl::setSamplingSettings>::prepare(
				tasksQueue
		);
		// schedule model change:
		return makeSetter<::setSamplingSettings>(
				tasksQueue,
				this->position,
				index, value
		);
	});
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
	if( expensiveTaskRunning ) {
		std::ranges::fill(buffer->begin(),buffer->end(), 0);
		return;
	}
	writeTasks.write([this,buffer,position,samplerate](auto& tasksQueue) {
	getNetworkConst()->read([this,buffer,position,samplerate,&tasksQueue](const auto& network) {
		network->valuesToBuffer(
				buffer,
				position, samplerate,
				[&tasksQueue,&network](auto position_sr, auto samplerate_sr) {
					Ramping::updateRamps(
							tasksQueue,
							network,
							position_sr,
							samplerate_sr
					);
				}
		);
	});
	});
}

bool ScheduledFunctionCollectionImpl::getAudioSchedulingEnabled() const
{
	LOG_FUNCTION()
	return audioSchedulingEnabled;
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
		writeTasks.write([](auto& tasksQueue) {
				Ramping::rampMasterEnv( tasksQueue, 1 );
		});
		return;
	}
	// switch off:
	else {
		// assert( masterVolumeEnv >= 0.99 );
		auto returnSignal = writeTasks.write([](auto &tasksQueue) {
			Ramping::rampMasterEnv( tasksQueue, 0 );
			return [&](){
				auto task = SignalReturnTask{};
				auto returnSignal = task.promise.get_future();
				tasksQueue.push_back(std::move(task));
				return returnSignal;
			}();
		});
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
	this->samplerate = samplerate;
	std::vector<std::function<void()>> parameteterSignals;
	writeTasks.try_write([&](auto& tasksQueue) -> void {
		// 1. 
		if( !tasksQueue.empty() ) {
			auto& someTask = tasksQueue.front();
			std::visit( [this,position,samplerate](auto& task) {
					using Task = std::decay_t<decltype(task)>;
					if constexpr ( IsSetterTask<Task>::value ) {
						if( !task.done ) {
							// delegate to the
							// model worker thread:
							modelTasks.release();
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
		for( auto& someTask : tasksQueue ) {
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
			else if( auto task = std::get_if<RampParameterTask>( &someTask ); task && task->done) {
				qDebug() << QString("%1: RampParameterTask done %2, %6: %3->%4, in %5 s")
					.arg( double(position) / double(samplerate) )
					.arg( task->index )
					.arg( task->src )
					.arg( task->dst )
					.arg( double(position - task->pos.value_or(position)) / double(samplerate) )
					.arg( task->parameterName )
				;
			}
		}
		#endif
		for( auto& someTask : tasksQueue ) {
			if( auto task = std::get_if<RampParameterTask>( &someTask ); task && task->done && task->succeeded ) {
				parameteterSignals.push_back( [
						signalizeDone = task->signalizeDone,
						index = task->index,
						name = task->parameterName,
						value = task->dst
				]() {
					signalizeDone(
						index,
						{ {
							name,
							{ C(value,0) }
						} }
					);
				});
			}
		}
		// cleanup ramps:
		{
			auto [rem_begin, rem_end] = std::ranges::remove_if(tasksQueue, [](auto& someTask) -> bool {
				return std::visit( [](auto& task) {
					using Task = std::decay_t<decltype(task)>;
					if constexpr ( IsSetterTask<Task>::value ) {
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
					else if constexpr ( std::is_same_v<Task,RampParameterTask> ) {
						return task.done;
					}
					return false;
				}, someTask );
			});
			tasksQueue.erase( rem_begin, rem_end );
		}
	});
	for( auto signal: parameteterSignals ) {
		signal();
	}
}

void ScheduledFunctionCollectionImpl::modelWorkerLoop()
{
	while(true) {
		modelTasks.acquire();
		if( stopModelWorker ) {
			break;
		}
		writeTasks.try_write([&](auto& tasksQueue) {
			assert( !tasksQueue.empty() );
			auto& someTask = tasksQueue.front();
			std::visit([&](auto& task) {
					using Task = std::decay_t<decltype(task)>;
					if constexpr ( IsSetterTask<Task>::value ) {
						assert( !task.done );
						{
							expensiveTaskRunning = true;
							getNetwork()->try_write([&task](auto network) {
								return run(network.get(), &task);
							});
							expensiveTaskRunning = false;
						}
						#ifdef LOG_MODEL
						qDebug() << QString("%1: executing '%2")
							.arg( double(position) / double(samplerate) )
							.arg( functionName(task) )
						;
						#endif
					}
					else {
						assert( IsSetterTask<Task>::value );
					}
			}, someTask );
		});
	};
}

/************************
 * Ramping
************************/

template <typename TaskQueue>
void Ramping::rampMasterEnv(
		TaskQueue& tasksQueue,
		double value
) {
	auto task = RampMasterEnvTask{ .dst = value };
	tasksQueue.push_back(std::move(task));
}

template <typename TaskQueue>
void Ramping::rampEntry(
		TaskQueue& tasksQueue,
		const uint index,
		double value
)
{
	auto task = RampTask{ .index = index, .dst = value };
	tasksQueue.push_back(std::move(task));
}

template <typename TaskQueue>
void Ramping::rampParameter(
		TaskQueue& tasksQueue,
		const uint index,
		const QString& parameterName,
		double value,
		ParameterSignalDone signalizeDone
)
{
	auto task = RampParameterTask{
		.index = index,
		.parameterName = parameterName,
		.dst = value,
		.signalizeDone = signalizeDone
	};
	tasksQueue.push_back(std::move(task));
}

template <typename TaskQueue>
void Ramping::adjustMasterVolume(
		TaskQueue& tasksQueue,
		const std::shared_ptr<SampledFunctionCollectionImpl> network
) {
	uint count = 0;
	{
		for(uint i=0; i<network->size(); i++ ) {
			if( network->getIsPlaybackEnabled(i) ){
				count++; 
			}
		}
	}
	{
		// ramp to scale master volume
		auto task = RampMasterVolumeTask{ .dst = 1.0/std::max(1.0,double(count)) };
		tasksQueue.push_back(std::move(task));
	}
}

template <typename TaskQueue>
void Ramping::updateRamps(
		TaskQueue& tasksQueue,
		const std::shared_ptr<SampledFunctionCollectionImpl> network,
		const PlaybackPosition position,
		const uint samplerate
) {
	// all recent entries which are ramps:
	auto rampView  = (
		tasksQueue
		| std::views::take_while([](auto& someTask) {
				return
					std::holds_alternative<RampTask>(someTask)
					|| std::holds_alternative<RampMasterEnvTask>(someTask)
					|| std::holds_alternative<RampMasterVolumeTask>(someTask)
					|| std::holds_alternative<RampParameterTask>(someTask)
				;
		})
		| std::views::reverse
	);
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
				[network](auto task) { return network->getMasterEnvelope(); },
				[network](auto task, const double value) { return network->setMasterEnvelope(value); },
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
				[network](auto task) { return network->getMasterVolume(); },
				[network](auto task, const double value) { return network->setMasterVolume(value); },
				position, samplerate
		);
	}
	// RampTask:
	for( uint index=0; index<network->size(); index++ )
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
				[network,index](auto task) { return network->getNodeInfo(index)->volumeEnvelope; },
				[network,index](auto task, const double value) { network->getNodeInfo(index)->volumeEnvelope = value; },
				position, samplerate
		);
	}
	// RampParameterTask:
	for( uint index=0; index<network->size(); index++ )
	{
		for( auto [parameterName,_] : network->get(index).parameters ) {
			auto view = rampView
				| std::views::filter([index,parameterName](auto& someTask){
					auto task = std::get_if<RampParameterTask>(&someTask);
					return
						task
						&& (task->index == index)
						&& (task->parameterName == parameterName)
						&& !task->done;
				})
				| std::views::transform([](auto& task){
					return &std::get<RampParameterTask>( task );
				})
			;
			updateRamp<RampParameterTask>(
					view,
					[network](auto task) -> double {
						return network->get(task->index).parameters.at(
								task->parameterName
						).at(0).c_.real();
					},
					[network,index](auto task, const double value) {
						network->setParameterValues(
								task->index,
								{ { task->parameterName, { C(value,0) } } }
						);
					},
					position, samplerate
			);
		}
	}
}

template <typename Task, typename View>
void Ramping::updateRamp(
		View view,
		std::function<double(const Task* task)> getValue,
		std::function<void(const Task* task,const double)> setValue,
		const PlaybackPosition position,
		const uint samplerate
) {
	if( !view.empty() )
	{
		Task* task = view.front();
		if( !task->pos) {
			task->pos = position;
			task->src = getValue(task);
		}
		double time = double(position - task->pos.value()) / double(samplerate);
		if constexpr ( std::is_same_v<Task,RampParameterTask> ) {
			double adjustedRampTime = parameterRampTime;
			if( time < adjustedRampTime )
			{
				setValue(
						task,
						task->src + (task->dst - task->src)
						* (time / adjustedRampTime)
				);
			}
			else {
				task->done = true;
				task->succeeded = true;
			}
		}
		else {
			double adjustedRampTime = rampTime;
			// double adjustedRampTime = fabs(task->src - task->dst) * rampTime;
			if( time < adjustedRampTime )
			{
				setValue(
						task,
						task->src + (task->dst - task->src)
						* (time / adjustedRampTime)
				);
			}
			else {
				task->done = true;
			}
		}
	}
	// ignore all previous tasksQueue:
	for( auto* task : view | std::ranges::views::drop(1) ) {
		// initialize tasksQueue if necessary
		// for correct debug output:
		if( !task->pos) {
			task->pos = position;
			task->src = getValue(task);
		}
		task->done = true;
	}
}
