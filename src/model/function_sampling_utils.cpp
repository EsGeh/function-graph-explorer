#include "fge/model/function_sampling_utils.h"


bool isBufferable(
		const SamplingSettings& samplingSettings
)
{
	return 
		samplingSettings.buffered
		&& samplingSettings.resolution != 0
		&& samplingSettings.periodic
	;
}

C getWithResolution(
		std::function<C(const C&)> function,
		const C& x,
		const SamplingSettings& samplingSettings,
		const FunctionBuffer* buffer
)
{
	std::function<C(const C)> periodicLookup = [function](const C x){ return function(x); };
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
			if( isBufferable( samplingSettings ) ) {
				const int xpos = xToRasterIndex(lookupPos, samplingSettings.resolution) % int(samplingSettings.resolution * samplingSettings.periodic);
				assert( buffer->inRange( xpos ) );
				return buffer->lookup( xpos );
			}
			return function( C(lookupPos,0) );
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
		std::function<C(const C&)> function,
		int x,
		const uint resolution
)
{
	return function(
			C(T(x) / resolution,0)
	);
}

void fillBuffer(
		std::function<C(const C&)> function,
		const SamplingSettings& samplingSettings,
		FunctionBuffer* buffer
)
{
	assert( isBufferable(samplingSettings) );
	buffer->fill(
			0, samplingSettings.resolution * samplingSettings.periodic,
			[function,samplingSettings](const int x) {
				return rasterIndexToY(function, x, samplingSettings.resolution);
			}
	);
}
