#include "fge/model/sampled_func_collection_impl.h"
#include "include/fge/model/cache.h"
#include "include/fge/model/function.h"
#include "include/fge/model/function_collection.h"
#include "include/fge/model/function_collection_impl.h"
#include "include/fge/model/sampled_func_collection.h"
#include "include/fge/model/function_sampling_utils.h"
#include <memory>
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
		const SamplingSettings& defSamplingSettings
)
	: FunctionCollectionImpl( defSamplingSettings )
{}

void SampledFunctionCollectionImpl::resize(const uint size)
{
	auto oldSize = this->size();
	LowLevel::resize( size );
	updateBuffers( oldSize );
}

// Read entries:
FunctionInfo SampledFunctionCollectionImpl::get(
		const Index index
) const
{
	LOG_FUNCTION()
	return LowLevel::getFunctionInfo(index);
}

MaybeError SampledFunctionCollectionImpl::getError(
		const Index index
) const
{
	LOG_FUNCTION()
	auto functionOrError = LowLevel::getFunction(index);
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
		FunctionInfo{
			.formula = formula,
			.parameters = parameters,
			.parameterDescriptions = get(index).parameterDescriptions,
			.stateDescriptions = stateDescriptions
		}
	);
	updateBuffers(index);
	return ret;
}

MaybeError SampledFunctionCollectionImpl::setParameterDescriptions(
		const Index index,
		const ParameterDescriptions& parameterDescriptions
)
{
	LOG_FUNCTION()
	auto old = get(index);
	auto ret = LowLevel::set( index, 
		FunctionInfo{
			.formula = old.formula,
			.parameters = old.parameters,
			.parameterDescriptions = parameterDescriptions,
			.stateDescriptions = old.stateDescriptions
		}
	);
	updateBuffers(index);
	return ret;
}

MaybeError SampledFunctionCollectionImpl::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	LOG_FUNCTION()
	auto ret = setParameterValuesDeferBufferUpdates( index, parameters ).first;
	updateBuffers(index);
	return ret;
}

std::pair<MaybeError, std::vector<SampledFunctionCollectionImpl::Index>> SampledFunctionCollectionImpl::setParameterValuesDeferBufferUpdates(
		const Index index,
		const ParameterBindings& parameters
)
{
	LOG_FUNCTION()
	auto ret = LowLevel::setParameterValues( index, parameters );
	std::vector<Index> buffered;
	for(uint i=index; i<size(); i++) {
		std::shared_ptr<Function> maybeFunction = nullptr;
		auto functionOrError = LowLevel::getFunction(i);
		if( functionOrError ) {
			maybeFunction = functionOrError.value();
		}
		const auto& samplingSettings = getSamplingSettings(i);
		if( maybeFunction && isBufferable(samplingSettings) ) {
			buffered.push_back( i );
		}
	}
	return { ret, buffered };
}


/***************
 * Sampling
 ***************/

// general settings:
SamplingSettings SampledFunctionCollectionImpl::getSamplingSettings(
		const Index index
) const
{
	return LowLevel::getSamplingSettings( index );
}

void SampledFunctionCollectionImpl::setSamplingSettings(
		const Index index,
		const SamplingSettings& value
)
{
	LOG_FUNCTION()
	LowLevel::setSamplingSettings( index, value );
	auto functionOrError = LowLevel::getFunction(index);
	functionOrError.transform([this,index](auto function) {
		updateBuffers(index);
	});
}

// sampling for visual representation:
ErrorOrValue<std::vector<std::pair<C,C>>> SampledFunctionCollectionImpl::getGraph(
		const Index index,
		const std::pair<T,T>& range,
		const unsigned int resolution
) const
{
	LOG_FUNCTION()
	auto errorOrFunction = LowLevel::getFunction( index );
	if( !errorOrFunction ) {
		return std::unexpected( errorOrFunction.error() );
	}
	else
	{
		auto func = errorOrFunction.value();
		auto
			xMin = range.first,
			xMax = range.second
		;
		std::vector<std::pair<C,C>> graph;
		for( unsigned int i=0; i<resolution; i++ ) {
			auto x = C( xMin + (T(i) / (resolution-1))*(xMax - xMin), 0);
			graph.push_back({
					x,
					func->get(x)
			});
		}
		return graph;
	}
}

// sampling for audio:

double SampledFunctionCollectionImpl::getPlaybackSpeed() const
{
	LOG_FUNCTION()
	return globalPlaybackSpeed;
}

void SampledFunctionCollectionImpl::setPlaybackSpeed( const double value )
{
	LOG_FUNCTION()
	globalPlaybackSpeed = value;
}

PlaybackSettings SampledFunctionCollectionImpl::getPlaybackSettings(
		const Index index
) const
{
	LOG_FUNCTION()
	return getNodeInfoConst(index)->playbackSettings;
}

void SampledFunctionCollectionImpl::setPlaybackSettings(
		const Index index,
		const PlaybackSettings& value
)
{
	LOG_FUNCTION()
	getNodeInfo(index)->playbackSettings = value;
}

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
		const unsigned int samplerate,
		AudioCallback callback
)
{
	for(
			PlaybackPosition pos=0;
			pos<buffer->size();
			pos++
	) {
		buffer->data()[pos] =
			audioFunction(position+pos, samplerate);
		callback( position+pos, samplerate );
	}
}

double SampledFunctionCollectionImpl::getMasterEnvelope() const 
{
	return masterEnvelope;
}

void SampledFunctionCollectionImpl::setMasterEnvelope(const double value)
{
	masterEnvelope = value;
}

double SampledFunctionCollectionImpl::getMasterVolume() const 
{
	return masterVolume;
}

void SampledFunctionCollectionImpl::setMasterVolume(const double value)
{
	masterVolume = value;
}

void SampledFunctionCollectionImpl::updateBuffers( const Index startIndex )
{
	for(uint index=startIndex; index<size(); index++) {
		std::shared_ptr<Function> maybeFunction = nullptr;
		auto functionOrError = LowLevel::getFunction(index);
		if( functionOrError ) {
			maybeFunction = functionOrError.value();
			functionOrError.value()->resetState();
		}
		updateBuffer( index, maybeFunction );
	}
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
		auto functionOrError = LowLevel::getFunction(i);
		auto isPlaybackEnabled = getNodeInfo(i)->isPlaybackEnabled;
		if( !functionOrError || !isPlaybackEnabled )
			continue;
		auto function = functionOrError.value();
		double volEnv = getNodeInfoConst(i)->volumeEnvelope;
		const auto& playbackSettings = getNodeInfoConst(i)->playbackSettings;
		ret += (
				function->get(
					time * globalPlaybackSpeed * playbackSettings.playbackSpeed
				).c_.real()
				* volEnv
		);
	}
	ret *= (masterEnvelope * masterVolume);
	return std::clamp( ret, -1.0, +1.0 );
}

std::shared_ptr<SampledFunctionCollectionImpl::LowLevel::NodeInfo> SampledFunctionCollectionImpl::createNodeInfo(
		const Index index,
		std::shared_ptr<Function> maybeFunction
)
{
	LOG_FUNCTION()
	auto ret = std::shared_ptr<NodeInfo>(new ::NodeInfo{});
	return ret;
};

void SampledFunctionCollectionImpl::updateBuffer(
		const Index index,
		std::shared_ptr<Function> maybeFunction
)
{
	if( maybeFunction ) {
		maybeFunction->update();
	}
}
