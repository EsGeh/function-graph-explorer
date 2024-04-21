#ifndef VIEWDATA_H
#define VIEWDATA_H

#include "fge/shared/utils.h"
#include <QString>


struct FunctionViewData {
	// QString formula;
	std::pair<T,T> origin = { 0, 0 };
	std::pair<T,T> scaleExp = { 0, 0 };
	std::pair<bool,bool> originCentered = { 0, 1 };
	bool displayImaginary = false;

	T playbackDuration = 1;
	T playbackSpeed = 1;
	T playbackOffset = 0;

	std::pair<T,T> getScale() const {
		return {
			pow( 2, scaleExp.first ),
			pow( 2, scaleExp.second )
		};
	}

	std::pair<T,T> getXRange() const {
		return {
			originCentered.first
				? origin.first - getScale().first
				: origin.first,
			origin.first + getScale().first
		};
	}

	std::pair<T,T> getYRange() const {
		return {
			originCentered.second
				? origin.second - getScale().second
				: origin.second,
			origin.second + getScale().second
		};
	}

};

#endif
