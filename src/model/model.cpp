#include "fge/model/model.h"
#include <mutex>
#include <strings.h>


inline QString functionName( const size_t index ) {
	return QString("f%1").arg( index );
}


#define DECL_FUNC_BEGIN(CLASS, ORD, ...) \
struct CLASS: \
	public exprtk::ifunction<C>  \
{ \
	CLASS() \
	: exprtk::ifunction<C>(ORD) \
	{} \
\
	C operator()( __VA_ARGS__ ) {

#define DECL_FUNC_END(CLASS) \
	} \
};

DECL_FUNC_BEGIN(RealFunction,1,const C& x)
	return C(std::real( x.c_ ));
DECL_FUNC_END(RealFunction)

DECL_FUNC_BEGIN(ImagFunction,1,const C& x)
	return C(std::imag( x.c_ ));
DECL_FUNC_END(ImagFunction)

DECL_FUNC_BEGIN(AbsFunction,1,const C& x)
	return C(std::abs( x.c_ ));
DECL_FUNC_END(AbsFunction)

DECL_FUNC_BEGIN(ArgFunction,1,const C& x)
	return C(std::arg( x.c_ ));
DECL_FUNC_END(ArgFunction)

DECL_FUNC_BEGIN(ConjFunction,1,const C& x)
	return C(std::conj( x.c_ ));
DECL_FUNC_END(ConjFunction)

DECL_FUNC_BEGIN(ComplexFunction,2,const C& x1, const C& x2)
	return C(T(x1), T(x2));
DECL_FUNC_END(ComplexFunction)

DECL_FUNC_BEGIN(PolarFunction,2,const C& x1, const C& x2)
	return C(std::polar( T(x1), T(x2) ));
DECL_FUNC_END(PolarFunction)

DECL_FUNC_BEGIN(RandomFunction,0,)
	return C(T(std::rand())/T(RAND_MAX), T(std::rand())/T(RAND_MAX));
DECL_FUNC_END(RandomFunction)

DECL_FUNC_BEGIN(Real_Compare,2,const C& x1, const C& x2)
	T ret = 0;
	if( x1.c_.real() > x2.c_.real() )
		ret = 1;
	else if( x1.c_.real() < x2.c_.real() )
		ret = -1;
	return C(ret, 0);
DECL_FUNC_END(Real_Compare)

static auto realFunc = RealFunction();
static auto imagFunc = ImagFunction();

static auto absFunc = AbsFunction();
static auto argFunc = ArgFunction();
static auto conjFunc = ConjFunction();
static auto complexFunc = ComplexFunction();
static auto polarFunc = PolarFunction();
static auto realCompare = Real_Compare();
static auto randomFunc = RandomFunction();

FuncNetworkImpl::FuncNetworkImpl()
	: constants(
		{
			{ "pi", C(acos(-1),0) },
			{ "e", cmplx::details::constant::e },
			{ "i", C(0,1) },
		},

		// functions:
		{
			{ "abs", &absFunc },
			{ "real", &realFunc },
			{ "imag", &imagFunc },
			{ "arg", &argFunc },
			{ "conj", &conjFunc },
			{ "complex", &complexFunc },
			{ "polar", &polarFunc },
			{ "real_cmp", &realCompare },
			{ "rnd", &randomFunc }
		}
	)
{}

uint FuncNetworkImpl::size() const
{
	return entries.size();
}

FunctionOrError FuncNetworkImpl::get(const uint index) const
{
	auto entry = entries.at( index );
	return entry->functionOrError
	.transform_error([](auto invalidEntry) {
			return invalidEntry.error;
	});
}

FunctionParameters FuncNetworkImpl::getFunctionParameters(const uint index) const
{
	auto entry = entries.at( index );
	if( entry->functionOrError ) {
		auto function = entry->functionOrError.value();
		return FunctionParameters{
			.formula = function->toString(),
			.parameters = function->getParameters(),
			.stateDescriptions = function->getStateDescriptions()
		};
	}
	else {
		auto error = entry->functionOrError.error();
		return error.parameters;
	}
}

bool FuncNetworkImpl::getIsPlaybackEnabled(
		const uint index
) const
{
	return entries.at( index )->isAudioEnabled;
}

void FuncNetworkImpl::setIsPlaybackEnabled(
		const uint index,
		const bool value
)
{
	auto entry = entries.at( index );
	entry->isAudioEnabled = value;
}


void FuncNetworkImpl::resize( const uint size ) {
	const auto oldSize = entries.size();
	if( size < oldSize ) {
		entries.resize( size );
	}
	else if( size > oldSize ) {
		for( uint i=oldSize; i<size; i++ ) {
			auto entry = std::shared_ptr<NetworkEntry>(new NetworkEntry {
					.functionOrError = std::unexpected(InvalidEntry{
						.error = "not yet initialized",
						.parameters = FunctionParameters{
								.formula = (i>0)
									? QString("%1(x)").arg( functionName(i-1) )
									: "cos( 2pi * x )"
						}
					})
			});
			entries.push_back( entry );
		}
		updateFormulas(oldSize, {});
	}
	assert( entries.size() == size );
}

MaybeError FuncNetworkImpl::set(
		const uint index,
		const FunctionParameters& parameters
) {
	// assert( index < size() );
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<index; i++ ) {
		auto entry = entries.at(i);
		if( entry->functionOrError.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
			);
		}
	}
	updateFormulas( index, parameters );
	if( !get(index) ) {
		return get(index).error();
	}
	return {};
}

MaybeError FuncNetworkImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	FunctionOrError functionOrError = get( index );
	if( !functionOrError ) {
		return functionOrError.error();
	}
	auto function = functionOrError.value();
	for( auto [key, val] : parameters ) {
		auto maybeError = function->setParameter( key, val );
		if( maybeError ) {
			return maybeError.value();
		}
	}
	return {};
}

void FuncNetworkImpl::updateFormulas(
		const size_t startIndex,
		const std::optional<FunctionParameters>& parameters
)
{
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = entries.at(i);
		if( entry->functionOrError.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
			);
		}
	}
	/* update entries
	 * from startIndex:
	 */
	for( size_t i=startIndex; i<entries.size(); i++ ) {
		auto entry = entries.at(i);
		FunctionParameters params = getFunctionParameters(i);
		if( i==startIndex && parameters ) {
			params = parameters.value();
		}
		entry->functionOrError = formulaFunctionFactory(
				params.formula,
				params.parameters,
				params.stateDescriptions,
				{
					constants,
					functionSymbols
				}
		).transform_error([params](auto error){
			return InvalidEntry{
				.error = error,
				.parameters = params
			};
		});
		if( entry->functionOrError ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
			);
		}
	}
}

/************************
 * FuncNetworkScheduled
************************/

#define START_READ() \
	if( audioSchedulingEnabled ) { \
		std::lock_guard lock( networkLock ); \
	}

#define LOCK_TASKS() \
	std::lock_guard lock( tasksLock );

FuncNetworkScheduled::FuncNetworkScheduled(
		const SamplingSettings& defSamplingSettings
)
{}

SamplingSettings FuncNetworkScheduled::getSamplingSettings(
		const Index index
)
{
	return no_optimization_settings;
}

void FuncNetworkScheduled::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{}

uint FuncNetworkScheduled::size() const
{
	START_READ()
	return getNetwork()->size();
}

void FuncNetworkScheduled::resize(const uint size)
{
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

FunctionOrError FuncNetworkScheduled::get(const Index index) const
{
	START_READ()
	return getNetwork()->get(index);
}

MaybeError FuncNetworkScheduled::set(
		const uint index,
		const FunctionParameters& parameters
)
{
	if( !audioSchedulingEnabled ) {
		return getNetwork()->set( index,parameters );
	}
	tasksLock.lock();
	// ramp down first:
	for(uint i=index; i<getNetwork()->size(); i++) {
		auto task = RampTask{ i, 1, 0 };
		writeTasks.push_back(std::move(task));
	}
	// set value:
	auto future = [this,index,parameters](){
		auto task = SetTask{
			index, parameters
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

MaybeError FuncNetworkScheduled::setParameterValues(
		const uint index,
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

FunctionParameters FuncNetworkScheduled::getFunctionParameters(const uint index) const
{
	START_READ()
	return getNetwork()->getFunctionParameters(index);
}

bool FuncNetworkScheduled::getIsPlaybackEnabled(
		const uint index
) const
{
	START_READ()
	return getNetwork()->getIsPlaybackEnabled(index);
}

void FuncNetworkScheduled::setIsPlaybackEnabled(
		const uint index,
		const bool value
)
{
	if( !audioSchedulingEnabled ) {
		return getNetwork()->setIsPlaybackEnabled( index,value );
	}
	tasksLock.lock();
	// ramp down first:
	if( !value )
	{
		volumeEnvelopes[index] = 1;
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
		volumeEnvelopes[index] = 0;
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

float FuncNetworkScheduled::audioFunction(
		const PlaybackPosition position,
		const uint samplerate
)
{
	double ret = 0;
	C time = C(T(position) / T(samplerate), 0);
	for( Index i=0; i<getNetwork()->size(); i++ ) {
		auto functionOrError = getNetwork()->get(i);
		auto isPlaybackEnabled = getNetwork()->getIsPlaybackEnabled(i);
		if( !functionOrError || !isPlaybackEnabled )
			continue;
		auto function = functionOrError.value();
		double volEnv = 1;
		if( auto it = volumeEnvelopes.find(i); it != volumeEnvelopes.end() ) {
			volEnv = it->second;
		}
		ret += (
				function->get( time ).c_.real()
				* volEnv
		);
	}
	ret *= masterVolumeEnv;
	return std::clamp( ret, -1.0, +1.0 );
}

ErrorOrValue<std::vector<std::pair<C,C>>> FuncNetworkScheduled::getGraph(
		const Index index,
		const std::pair<T,T>& range,
		const unsigned int resolution
)
{
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

void FuncNetworkScheduled::updateRamps(
		const PlaybackPosition position,
		const uint samplerate
)
{
	this->position = position;
	{
	{
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
					writeTasks.pop_front();
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
					volumeEnvelopes[task->index] = env;
				}
				else {
					writeTasks.pop_front();
				}
			}
		}
	}
	}
}

void FuncNetworkScheduled::executeWriteOperations(
		const PlaybackPosition position,
		const uint samplerate
)
{
	this->position = position;
	// only write, if no one is reading:
	if( networkLock.try_lock() ) {
		// only write, if no one is adding a task:
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
					getNetwork()->setIsPlaybackEnabled(
							task->index,
							task->value
					);
					task->promise.set_value();
					writeTasks.pop_front();
				}
				else if( auto task = std::get_if<SignalReturnTask>(&someTask) ) {
					task->promise.set_value();
					writeTasks.pop_front();
				}
			}
			tasksLock.unlock();
		}
		networkLock.unlock();
	}
}

void FuncNetworkScheduled::setAudioSchedulingEnabled(
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
		assert( masterVolumeEnv >= 0.99 );
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
		assert( masterVolumeEnv < 0.01 );
		audioSchedulingEnabled = value;
		return;
	}
	else {
		assert( false );
	}
}

double FuncNetworkScheduled::currentEnvVal(const Index index) const
{
	if( auto it = volumeEnvelopes.find(index); it != volumeEnvelopes.end() ) {
		return it->second;
	}
	return 1;
}
