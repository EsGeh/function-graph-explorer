#include "fge/model/sampled_func_collection_impl.h"
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
		const SamplingSettings& defSamplingSettings,
		AudioCallback audioCallback 
)
	: defSamplingSettings( defSamplingSettings )
	, audioCallback( audioCallback )
{}

// Read entries:
FunctionParameters SampledFunctionCollectionImpl::get(
		const size_t index
) const
{
	LOG_FUNCTION()
	return LowLevel::getFunctionParameters(index);
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
	if( value.caching ) {
		std::shared_ptr<Function> maybeFunction = nullptr;
		auto functionOrError = LowLevel::get(index);
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
			auto functionOrError = LowLevel::get(i);
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
