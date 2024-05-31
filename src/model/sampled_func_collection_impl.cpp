#include "fge/model/sampled_func_collection_impl.h"
#include "include/fge/model/cache.h"
#include "include/fge/model/function.h"
#include "include/fge/model/function_collection.h"
#include <cstddef>
#include <ctime>
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
	: defSamplingSettings( defSamplingSettings )
{}

void SampledFunctionCollectionImpl::resize(const uint size)
{
	auto oldSize = this->size();
	LowLevel::resize( size );
	updateBuffers( oldSize );
}

// Read entries:
FunctionInfo SampledFunctionCollectionImpl::get(
		const size_t index
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
		FunctionInfo{
			.formula = formula,
			.parameters = parameters,
			.stateDescriptions = stateDescriptions
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
	auto ret = LowLevel::setParameterValues( index,parameters );
	updateBuffers(index);
	return ret;
}


/***************
 * Sampling
 ***************/

// general settings:
SamplingSettings SampledFunctionCollectionImpl::getSamplingSettings(
		const Index index
) const
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
	if( value.buffered ) {
		std::shared_ptr<Function> maybeFunction = nullptr;
		auto functionOrError = LowLevel::get(index);
		if( functionOrError ) {
			maybeFunction = functionOrError.value();
		}
		updateBuffer(
				index,
				maybeFunction
		);
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
					&nodeInfo->functionBuffer
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
		double volEnv = getNodeInfoConst(i)->volumeEnvelope;
		const auto& samplingSettings = getNodeInfoConst(i)->samplingSettings;
		const auto& buffer = getNodeInfoConst(i)->functionBuffer;
		ret += (
				getWithResolution(
					function,
					time,
					samplingSettings,
					&buffer
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
	auto ret = std::shared_ptr<NodeInfo>(new ::NodeInfo{
			.samplingSettings = defSamplingSettings,
	});
	return ret;
};

void SampledFunctionCollectionImpl::updateBuffers( const Index startIndex )
{
	for(uint index=startIndex; index<size(); index++) {
		std::shared_ptr<Function> maybeFunction = nullptr;
		auto functionOrError = LowLevel::get(index);
		if( functionOrError ) {
			maybeFunction = functionOrError.value();
		}
		updateBuffer( index, maybeFunction );
	}
}

void SampledFunctionCollectionImpl::updateBuffer(
		const Index index,
		std::shared_ptr<Function> maybeFunction
)
{
	auto& buffer = getNodeInfo(index)->functionBuffer;
	const auto& samplingSettings = getNodeInfoConst(index)->samplingSettings;
	if( isBufferable(maybeFunction, samplingSettings) ) {
		buffer.fill(
				0, samplingSettings.resolution * samplingSettings.periodic,
				[maybeFunction,samplingSettings](const int x) {
					return rasterIndexToY(maybeFunction, x, samplingSettings.resolution);
				}
		);
	}
}

bool isBufferable(
		std::shared_ptr<Function> function,
		const SamplingSettings& samplingSettings
)
{
	return 
		function
		&& samplingSettings.buffered
		&& samplingSettings.resolution != 0
		&& samplingSettings.periodic
	;
}

C getWithResolution(
		std::shared_ptr<Function> function,
		const C& x,
		const SamplingSettings& samplingSettings,
		const FunctionBuffer* buffer
)
{
	std::function<C(const C)> periodicLookup = [function](const C x){ return function->get(x); };
	if( samplingSettings.periodic != 0 && x.c_.imag() == 0 ) {
		periodicLookup = [function,samplingSettings,buffer](const C x) {
			T lookupPos = [&](){
				const auto mod = fmod(x.c_.real(), samplingSettings.periodic);
				if( x.c_.real() >= 0 )
					return mod;
				return samplingSettings.periodic + mod;
			}();
			/*
			assert( lookupPos >= 0 );
			assert( lookupPos < samplingSettings.periodic );
			*/
			if( isBufferable( function, samplingSettings ) ) {
				const int xpos = xToRasterIndex(lookupPos, samplingSettings.resolution) % samplingSettings.resolution;
				assert( buffer->inRange( xpos ) );
				return buffer->lookup( xpos );
			}
			return function->get( C(lookupPos,0) );
		};
	};
	if(
			samplingSettings.resolution == 0
			|| x.c_.imag() != 0
	) {
		return periodicLookup(x);
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
		const int xpos = xToRasterIndex(x.c_.real(), samplingSettings.resolution) + i - shift;
		ys.push_back(periodicLookup(
				C(
					T(xpos) / samplingSettings.resolution,
					0
				)
		));
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
		const T x,
		const uint resolution
)
{
		return std::floor(x * resolution + epsilon);
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
