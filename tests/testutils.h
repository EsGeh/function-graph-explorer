#pragma once

#include "fge/model/model.h"
#include <QTest>
#include <ranges>


#define FUZZY_CMP_DOUBLE(a,b) \
	(qFuzzyCompare(a,b) \
	|| qFuzzyCompare(b+2,2))

#define FUZZY_CMP_C(a,b) \
	(FUZZY_CMP_DOUBLE( a.c_.real(), b.c_.real() ) \
	&& FUZZY_CMP_DOUBLE( a.c_.imag(), b.c_.imag() ))

#define ASSERT_FUNC_POINT( X,Y, expectedY ) \
	QVERIFY2( \
		FUZZY_CMP_C( Y, expectedY ), \
		QString( "ERROR at function point: %1 -> %2 != %3 (expected)" ) \
			.arg( to_qstring( X ) ) \
			.arg( to_qstring( Y ) ) \
			.arg( to_qstring( expectedY ) ) \
		.toStdString().c_str() \
	)

/* utilities */

inline void initTestModel(
		Model* model,
		const std::vector<QString>& testData
) {
	model->resize(testData.size());
	for( uint i=0; i<testData.size(); i++ ) {
		auto funcString = testData[i] ;
		model->set(i, funcString, {}, {});
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
