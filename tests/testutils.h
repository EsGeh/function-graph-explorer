#pragma once

#include "fge/model/model.h"
#include <QTest>
#include <ranges>

/* utilities */

inline void initTestModel(
		Model* model,
		const std::vector<QString>& testData
) {
	model->resize(testData.size());
	for( uint i=0; i<testData.size(); i++ ) {
		auto funcString = testData[i] ;
		model->set(i, funcString, {} );
	}
}

inline void initTestModel(
		Model* model,
		const std::vector<std::pair<QString,std::function<C(T)>>>& testData
) {
	auto view = testData | std::ranges::views::elements<0>;
	std::vector vec( view.begin(), view.end() );
	initTestModel( model, vec );
}
