#pragma once

#include "fge/model/function.h"


bool isBufferable(
		const SamplingSettings& samplingSettings
);

C getWithResolution(
		std::function<C(const C&)> function,
		// Function* function,
		const C& x,
		const SamplingSettings& samplingSettings,
		const FunctionBuffer* buffer
);

C interpolate(
		const C& x,
		const std::vector<C> ys,
		const int shift,
		const uint resolution

);

int xToRasterIndex(
		const T x,
		const uint resolution
);

C rasterIndexToY(
		std::function<C(const C&)> function,
		// Function* function,
		int x,
		const uint resolution
);

void fillBuffer(
		std::function<C(const C&)> function,
		const SamplingSettings& samplingSettings,
		FunctionBuffer* buffer
);
