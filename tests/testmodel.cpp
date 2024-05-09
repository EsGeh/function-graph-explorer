#include "testmodel.h"
#include "testutils.h"
#include <qcoreapplication.h>
#include <qfloat16.h>
#include <qtestcase.h>
#include <stdexcept>

QTEST_MAIN(TestModel)
#include "testmodel.moc"


// UTILS:

void assertAllFunctionsValid(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
);

void assertCorrectGraph(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
);

/* TEST */

void TestModel::testInit() {
	Model model;
	QCOMPARE( model.size(), 0 );
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getFormula( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getError( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getIsPlaybackEnabled( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getGraph( model.size(), {0,1}, 8)
	);
}

void TestModel::testResizeUp() {
	Model model;
	model.resize(3);
	QCOMPARE( model.size(), 3 );
	for( uint i=0; i<model.size(); i++ ) {
		QVERIFY_THROWS_NO_EXCEPTION(
			model.getFormula( i )
		);
		QVERIFY_THROWS_NO_EXCEPTION(
			model.getError( i )
		);
		QVERIFY_THROWS_NO_EXCEPTION(
			model.getIsPlaybackEnabled( i )
		);
		QVERIFY_THROWS_NO_EXCEPTION(
			model.getGraph( i, {0,1}, 8)
		);
	}
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getFormula( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getError( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getIsPlaybackEnabled( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getGraph( model.size(), {0,1}, 8)
	);
}

void TestModel::testResizeDown() {
	Model model;
	model.resize(3);
	auto oldFormula = model.getFormula(0);
	auto oldError = model.getError(0);
	auto oldIsPlaybackEnabled = model.getIsPlaybackEnabled(0);
	auto oldGraph = model.getGraph(0, {0,1},8);
	model.resize(1);
	QCOMPARE( model.size(), 1 );
	/* downsizing invalidates
	 * entries from the tail,
	 * but leaves the head
	 * untouched
	 */
	{
		auto newFormula = model.getFormula(0);
		QCOMPARE( model.getFormula(0), oldFormula );
		QCOMPARE( model.getError(0), oldError );
		QCOMPARE( model.getIsPlaybackEnabled(0), oldIsPlaybackEnabled );
		QCOMPARE( model.getGraph(0,{0,1},8), oldGraph );
	}
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getFormula( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getError( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getIsPlaybackEnabled( model.size() )
	);
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getGraph( model.size(), {0,1}, 8)
	);
}

void TestModel::testSetEntry() {
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x^1", [](auto x){ return C(x,0); } },
		{ "x^2", [](auto x){ return C(x*x,0); } },
		{ "x^3", [](auto x){ return C(x*x*x,0); } }
	};
	initTestModel( &model, testData );
	assertAllFunctionsValid( model, testData );
	assertCorrectGraph( model, testData );
}

void TestModel::testFunctionReferences() {
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x-1", [](T x){ return C(x-1, 0); } },
		{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
	};
	initTestModel( &model, testData );
	assertAllFunctionsValid( model, testData );
	assertCorrectGraph( model, testData );
}

void TestModel::testUpdatesReferences() {
	Model model;
	// initial state:
	{
		std::vector<std::pair<QString, std::function<C(T)>>> testData = {
			{ "x-1", [](T x){ return C(x-1, 0); } },
			{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
		};
		initTestModel( &model, testData );
	}
	/* update F0:
	 */
	model.set( 0, "x+1", {}, {});
	/* Check result:
	 */
	std::vector<std::pair<QString, std::function<C(T)>>> expectedResult = {
		{ "x+1", [](T x){ return C(x+1, 0); } },
		{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x+1), 0); } }
	};
	assertAllFunctionsValid( model, expectedResult );
	assertCorrectGraph( model, expectedResult );
}

void TestModel::testGetGraph()
{
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x-1", [](T x){ return C(x-1, 0); } },
		{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
	};
	initTestModel( &model, testData );
	assertCorrectGraph( model, testData );
}

void TestModel::testValuesToBuffer()
{
	Model model;
	const double pi = asin(1);
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "cos(440 * 2pi * x)", [pi](T x){ return C(cos(440 * 2*pi * x), 0); } },
	};
	const uint samplerate = 44100;
	initTestModel( &model, testData );
	std::vector<float> buffer;
	buffer.resize( samplerate );

	// if playback not enabled,
	// buffer should be all zeroes:
	model.valuesToBuffer( 
			&buffer,
			0,
			samplerate
	);
	QVERIFY( buffer.size() == samplerate );
	QVERIFY( buffer == std::vector<float>(samplerate, 0) );

	// after enabling playback
	// the buffer should contain sth:
	model.setIsPlaybackEnabled( 0, true );
	model.valuesToBuffer( 
			&buffer,
			0,
			samplerate
	);
	QVERIFY( buffer.size() == samplerate );
	QVERIFY( buffer != std::vector<float>(samplerate, 0) );
}


/* utilities */

void assertAllFunctionsValid(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
)
{
	for( uint i=0; i<expectedResult.size(); i++ ) {
		auto maybeError = model.getError( i );
		auto errStr = maybeError ? maybeError.value() : "";
		QVERIFY2( !maybeError, 
			QString("error in expression: %1")
				.arg( errStr )
			.toStdString().c_str()
		);
		QVERIFY( model.getGraph(i, {0,1}, 8) );
	}
}

void assertCorrectGraph(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
)
{
	const unsigned int resolution = 7;
	const std::pair<T,T> range = {-3, 3};
	for( uint iFunction=0; iFunction<expectedResult.size(); iFunction++ ) {
		auto expectedString = expectedResult[iFunction].first ;
		auto expectedFunc = expectedResult[iFunction].second ;
		auto errOrGraph = model.getGraph(
				iFunction,
				range,
				resolution
		);
		{
			const auto error = 
					!errOrGraph
					? errOrGraph.error()
					: ""
			;
			QVERIFY2(
					errOrGraph,
					QString("error in expression: %1")
						.arg( error )
					.toStdString().c_str()
			);
		}
		auto graph = errOrGraph.value();

		QCOMPARE_GE( graph.size(), resolution );

		// x[0], x[max]
		{
			QCOMPARE( graph[0].first, range.first );
			QCOMPARE( graph[graph.size()-1].first, range.second );
		}
		// y[0], y[max]
		{
			const C y0 = expectedFunc( range.first );
			const C yMax = expectedFunc( range.second );
			ASSERT_FUNC_POINT( C(range.first,0), graph[0].second, y0 );
			ASSERT_FUNC_POINT( C(range.second,0), graph[graph.size()-1].second, yMax );
		}

		// constraints on x-subdivision:
		for( unsigned int i=0; i<graph.size()-1; i++ ) {
			QCOMPARE_LT( graph[i].first.c_.real(), graph[i+1].first.c_.real() );
			QCOMPARE_LT(
					graph[i+1].first.c_.real() - graph[i].first.c_.real(),
					(range.second-range.first) / graph.size() * 2
			);
			QCOMPARE_EQ( graph[i].first.c_.imag(), 0 );
		}

		// y values:
		std::vector<std::pair<C,C>> expectedGraph;
		for( auto point : graph ) {
			auto expectedY = expectedFunc( point.first );
			ASSERT_FUNC_POINT( point.first, point.second, expectedY );
		}
	}
}
