#include "fge/model/model_impl.h"
#include "include/fge/model/sampled_func_collection.h"
#include "include/fge/model/sampled_func_collection_impl.h"
#include "include/fge/model/template_utils.h"
#include "include/fge/model/template_utils_def.h"
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <strings.h>
#include <QDebug>
#include <ranges>
#include <algorithm>
#include <thread>


#ifndef NDEBUG
#define LOG_MODEL
#endif

#ifdef LOG_MODEL_GET
#include <source_location>
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

#ifdef LOG_MODEL_GET
#define LOG_FUNCTION_GET() \
	{ \
		const auto location = std::source_location::current(); \
		qDebug() << QString("%1: %2").arg(double(position) / 44100.0).arg(location.function_name()); \
	}
#else
#define LOG_FUNCTION_GET()
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
	), "NETWORK" )
	, writeTasks( "TASKS" )
	, modelWorkerThread([this](){ modelWorkerLoop(); } )
{
	LOG_FUNCTION()
}

ScheduledFunctionCollectionImpl::~ScheduledFunctionCollectionImpl()
{
	LOG_FUNCTION()
	{
		std::unique_lock lock( writeTasksSignal.lock );
		qDebug() << "kill MODEL WORKER";
		writeTasksSignal.stopModelWorker = true;
		writeTasksSignal.condition_var.notify_one();
	}
	qDebug() << "join MODEL WORKER...";
	modelWorkerThread.join();
	qDebug() << "join MODEL WORKER done";
}

/************************
 * READ:
************************/

ScheduledFunctionCollectionImpl::Index ScheduledFunctionCollectionImpl::size() const
{
	LOG_FUNCTION_GET()
	return getNetworkConst()->read([](auto& network){ return network->size(); });
}

// Read entries:
FunctionInfo ScheduledFunctionCollectionImpl::get(
		const Index index
) const
{
	LOG_FUNCTION_GET()
	return getNetworkConst()->read([index](auto& network) {
			return network->get(index);
	});
}

MaybeError ScheduledFunctionCollectionImpl::getError(
		const Index index
) const
{
	LOG_FUNCTION_GET()
	return getNetworkConst()->read([index](auto& network){
			return network->getError(index);
	});
}

SamplingSettings ScheduledFunctionCollectionImpl::getSamplingSettings(
		const Index index
) const
{
	LOG_FUNCTION_GET()
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
	LOG_FUNCTION_GET()
	return getNetworkConst()->read([index,range,resolution](auto& network){
			return network->getGraph(index, range, resolution);
	});
}

// sampling for audio:

PlaybackSettings ScheduledFunctionCollectionImpl::getPlaybackSettings(
		const Index index
) const
{
	LOG_FUNCTION_GET()
	return getNetworkConst()->read([index](auto& network){
			return network->getPlaybackSettings(index);
	});
}

bool ScheduledFunctionCollectionImpl::getIsPlaybackEnabled(
		const Index index
) const
{
	LOG_FUNCTION_GET()
	return getNetworkConst()->read([index](auto& network){
			return network->getIsPlaybackEnabled(index);
	});
}

double ScheduledFunctionCollectionImpl::getPosition() const
{
	// LOG_FUNCTION_GET()
	return this->position;
}

uint ScheduledFunctionCollectionImpl::getSamplerate() const
{
	// LOG_FUNCTION_GET()
	return this->samplerate;
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
				[]{},
				size
		);
	});
	future.get();
	return;
}

std::future<MaybeError> toMaybeError( std::future<void>&& f ) {
	return std::async( std::launch::deferred, [f = std::move(f)] -> MaybeError {
			f.wait();
			return {};
	} );
};

MaybeError ScheduledFunctionCollectionImpl::bulkUpdate(
		const Index index,
		const Update& update
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->write([index,&update](auto& network) {
			MaybeError ret{};
			if(
					update.formula.has_value()
					|| update.parameters.has_value()
					|| update.stateDescriptions.has_value()
			) {
				ret = network->set(
						index,
						update.formula.value_or( network->get(index).formula ),
						update.parameters.value_or( network->get(index).parameters ),
						update.stateDescriptions.value_or( network->get(index).stateDescriptions )
				);
			}
			if( update.parameterDescriptions.has_value() ) {
				network->setParameterDescriptions( index, update.parameterDescriptions.value() );
			}
			if( update.playbackSettings.has_value() ) {
				network->setPlaybackSettings(index, update.playbackSettings.value());
			}
			if( update.playbackEnabled.has_value() ) {
				network->setIsPlaybackEnabled(index, update.playbackEnabled.value());
			}
			if( update.samplingSettings.has_value() ) {
				network->setSamplingSettings(index, update.samplingSettings.value());
			}
			return ret;
		});
	}
	// audioSchedulingEnabled => update with ramping:
	update.parameterDescriptions.and_then([&](const auto& descrs) {
		getNetwork()->read([&](auto& network) {
			network->setParameterDescriptions( index, descrs );
		});
		return std::optional<ParameterBindings>{};
	});
	auto futures = writeTasks.write([&](auto& tasksQueue)
			-> std::vector<std::future<MaybeError>>
	{
		// prepare:
		if(
				update.formula.has_value()
				|| update.parameters.has_value()
				|| update.stateDescriptions.has_value()
		) {
			WritePrepare<&ScheduledFunctionCollectionImpl::set>::prepare(tasksQueue);
		}
		if( update.playbackEnabled.has_value() ) {
			getNetwork()->read([&tasksQueue,index,value = update.playbackEnabled.value()](auto& network) {
					WritePrepare<&ScheduledFunctionCollectionImpl::setIsPlaybackEnabled>::prepare(
							tasksQueue,
							network,
							index, value
					);
			});
		}
		if( update.playbackSettings.has_value() ) {
			WritePrepare<&ScheduledFunctionCollectionImpl::setPlaybackSettings>::prepare(
					tasksQueue
			);
		}
		if( update.samplingSettings.has_value() ) {
			WritePrepare<&ScheduledFunctionCollectionImpl::setSamplingSettings>::prepare(
					tasksQueue
			);
		}
		// SET:
		return [this,&tasksQueue,index,update]()
			-> std::vector<std::future<MaybeError>>
		{
			std::vector<std::future<MaybeError>> ret;
			if(
					update.formula.has_value()
					|| update.parameters.has_value()
					|| update.stateDescriptions.has_value()
			) {
				ret.push_back( std::move( makeSetter<::set>(
							tasksQueue,
							this->position,
							[]{},
							index,
							update.formula.value_or( get(index).formula ),
							update.parameters.value_or( get(index).parameters ),
							update.stateDescriptions.value_or( get(index).stateDescriptions )
				) ) );
			}
			if( update.playbackSettings.has_value() ) {
				ret.push_back( toMaybeError( makeSetter<::setPlaybackSettings>(
						tasksQueue,
						this->position,
						[]{},
						index, update.playbackSettings.value()
				) ) );
			}
			if( update.playbackEnabled.has_value() ) {
				ret.push_back( toMaybeError( makeSetter<::setIsPlaybackEnabled>(
						tasksQueue,
						this->position,
						[]{},
						index, update.playbackEnabled.value()
				) ) );
			}
			if( update.samplingSettings.has_value() ) {
				ret.push_back( toMaybeError( makeSetter<::setSamplingSettings>(
						tasksQueue,
						this->position,
						[]{},
						index, update.samplingSettings.value()
				) ) );
			}
			return ret;
		}();
	});
	MaybeError ret = {};
	for( auto& f : futures ) {
		auto currentRet = f.get();
		if( currentRet ) {
			ret = currentRet;
		}
	}
	writeTasks.write([&](auto& tasksQueue) {
		getNetwork()->read([&tasksQueue](auto& network) {
			postWrite(
					tasksQueue,
					network
			);
		});
	});
	return ret;
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
	auto future = writeTasks.write([&](auto& tasksQueue) {
		// ramp down first:
		WritePrepare<&ScheduledFunctionCollectionImpl::set>::prepare(tasksQueue);
		// schedule model change:
		return makeSetter<::set>(
					tasksQueue,
					this->position,
					[]{},
					index, formula, parameters, stateDescriptions
		);
	});
	auto ret = future.get();
	return ret;
}

MaybeError ScheduledFunctionCollectionImpl::setParameterDescriptions(
		const Index index,
		const ParameterDescriptions& parameterDescriptions
)
{
	LOG_FUNCTION()
	return getNetwork()->write([index,&parameterDescriptions](auto& network) {
			return network->setParameterDescriptions( index, parameterDescriptions );
	});
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
	auto future = writeTasks.write([&](auto& tasksQueue)
			-> std::future<MaybeError>
	{
		return getNetwork()->read([&](auto network) {
			auto volumeFadeParametersView =
				parameters
				| std::ranges::views::filter( [network,index](const auto& entry) {
						return network->get(index).parameterDescriptions.at(
							entry.first
						).rampType == FadeType::RampVolume;
				} )
			;
			if( volumeFadeParametersView.empty() ) {
				std::promise<MaybeError> promise;
				auto future = promise.get_future();
				promise.set_value({});
				return future;
			}
			// ramp down first:
			WritePrepare<&ScheduledFunctionCollectionImpl::setParameterValues>::prepare(tasksQueue);
			ParameterBindings volumeFadeParameters;
			for( auto [name, value] : volumeFadeParametersView ) {
				// qDebug() << "FADE parameter:" << name;
				volumeFadeParameters.insert({ name, value });
			}
			// schedule model change:
			return makeSetter<::setParameterValues>(
					tasksQueue,
					this->position,
					[]{},
					index, volumeFadeParameters
			);
		});
	});
	return future.get();
}

ParameterBindings ScheduledFunctionCollectionImpl::scheduleSetParameterValues(
		const Index index,
		const ParameterBindings& parameters,
		ParameterSignalDone signalizeDone
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return {};
	}
	return writeTasks.write([&](auto& tasksQueue){
		// find all parameters which are to be faded by
		// ramping the parameter
		ParameterBindings selectedParams = getNetwork()->read([&](auto network) {
			return parameters
				| std::ranges::views::filter([&](auto entry) {
						auto params = network->get(index).parameterDescriptions;
						if( params.find( entry.first ) == params.end() ) {
							return false;
						}
						return params.at( entry.first ).rampType == FadeType::RampParameter;
					})
				| std::ranges::to<ParameterBindings>();
		});
		// ramp parameters:
		for( auto [parameterName, value] : selectedParams ) {
			Ramping::rampParameter(
				tasksQueue,
				index,
				parameterName,
				value.c_.real(),
				signalizeDone
			);
		}
		return selectedParams;
	});
}

void ScheduledFunctionCollectionImpl::setPlaybackSettings(
		const Index index,
		const PlaybackSettings& value
)
{
	LOG_FUNCTION()
	if( !audioSchedulingEnabled ) {
		return getNetwork()->write([index,value](auto& network) {
			return network->setPlaybackSettings( index, value );
		});
		return;
	}
	auto future = writeTasks.write([&](auto& tasksQueue) {
		// ramp down first:
		WritePrepare<&ScheduledFunctionCollectionImpl::setPlaybackSettings>::prepare(
				tasksQueue
		);
		// schedule model change:
		return makeSetter<::setPlaybackSettings>(
				tasksQueue,
				this->position,
				[]{},
				index,value
		);
	});
	future.get();
	return;
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
				[]{},
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
				[]{},
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
	// LOG_FUNCTION_GET()
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
		auto returnSignal = writeTasks.write([this](auto& tasksQueue) {
			Ramping::rampMasterEnv( tasksQueue, 1 );
			getNetwork()->read([&tasksQueue](auto& network) {
				Ramping::adjustMasterVolume(tasksQueue, network);
			});
			return [&](){
				auto task = SignalReturnTask{};
				auto returnSignal = task.promise.get_future();
				tasksQueue.push_back(std::move(task));
				return returnSignal;
			}();
		});
		returnSignal.get();
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
							std::unique_lock lock( writeTasksSignal.lock );
							writeTasksSignal.pendingTask = true;
							writeTasksSignal.condition_var.notify_one();
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

		auto finishedRamps = tasksQueue
				| std::views::filter([](auto& someTask){
					auto task = std::get_if<RampParameterTask>(&someTask);
					return task && task->done && task->succeeded;
				})
				| std::views::transform([](auto& task){
					return &std::get<RampParameterTask>( task );
				})
				| std::ranges::to<std::vector<RampParameterTask*>>()
		;
		for( auto task : finishedRamps ) {
			if( !task->delayedUpdates.empty() ) {
				WritePrepare<&ScheduledFunctionCollectionImpl::setParameterValues>::prepare(tasksQueue);
			}
			makeSetter<::updateBuffers>(
					tasksQueue,
					this->position,
					[ signalizeDone = task->signalizeDone
					, index = task->index
					, name = task->parameterName
					, value = task->dst
					]{
						signalizeDone(
								index,
								{ {
									name,
									{ C(value,0) }
								} }
						);
					},
					task->index
			);
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
}

void ScheduledFunctionCollectionImpl::modelWorkerLoop()
{
	qDebug() << "MODEL WORKER THREAD: start";
	while(true) {
		{
			std::unique_lock lock( writeTasksSignal.lock );
			writeTasksSignal.condition_var.wait(
					lock,
					[this]{ return
						writeTasksSignal.pendingTask
						|| writeTasksSignal.stopModelWorker
						;
					}
			);
		}
		if( writeTasksSignal.stopModelWorker ) {
			qDebug() << "MODEL WORKER THREAD: stop:";
			break;
		}
		qDebug() << "MODEL WORKER THREAD: woke up";
		TaskDoneCallback taskDoneCallback = writeTasks.write([&](auto& tasksQueue) {
			assert( !tasksQueue.empty() );
			auto& someTask = tasksQueue.front();
			return std::visit([&](auto& task)
					-> TaskDoneCallback
			{
					using Task = std::decay_t<decltype(task)>;
					assert( IsSetterTask<Task>::value );
					if constexpr ( IsSetterTask<Task>::value ) {
						assert( !task.done );
						expensiveTaskRunning = true;
						auto ret = getNetwork()->write([&task](auto network) {
							return run(network.get(), &task);
						});
						expensiveTaskRunning = false;
						#ifdef LOG_MODEL
						qDebug() << QString("%1: executing '%2")
							.arg( double(position) / double(samplerate) )
							.arg( functionName(task) )
						;
						#endif
						task.done = true;
						return ret;
					}
					return {};
			}, someTask );
		});
		taskDoneCallback();
		writeTasksSignal.pendingTask = false;
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
						).c_.real();
					},
					[network,index](auto task, const double value) {
						task->delayedUpdates =
							network->setParameterValuesDeferBufferUpdates(
									task->index,
									{ { task->parameterName, { C(value,0) } } }
							).second;
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
		std::function<void(Task* task,const double)> setValue,
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
			std::vector<Index> delayedUpdates;
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
