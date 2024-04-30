#include "testmodel.h"
#include "testutils.h"
#include <qcoreapplication.h>
#include <qfloat16.h>
#include <qtestcase.h>
#include <stdexcept>

QTEST_MAIN(TestModel)
#include "testmodel.moc"

/* TEST */

void TestModel::testInit() {
	Model model;
	QCOMPARE( model.size(), 0 );
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getFunction( model.size() )
	);
}

void TestModel::testResizeUp() {
	Model model;
	model.resize(3);
	QCOMPARE( model.size(), 3 );
	for( uint i=0; i<model.size(); i++ ) {
		QVERIFY_THROWS_NO_EXCEPTION(
			model.getFunction( i );
		);
		/* elements are initialized
		 * with a valid default
		 * function
		 */
		auto errOrFunc = model.getFunction( i );
		{
			const auto error = 
					!errOrFunc
					? errOrFunc.error()
					: ""
			;
			QVERIFY2(
					errOrFunc,
					QString("error in expression: %1")
						.arg( error )
					.toStdString().c_str()
			);
		}
	}
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getFunction( model.size() )
	);
}

void TestModel::testResizeDown() {
	Model model;
	model.resize(3);
	auto entryOld = model.getFunction(0);
	model.resize(1);
	QCOMPARE( model.size(), 1 );
	/* downsizing invalidates
	 * entries from the tail,
	 * but leaves the head
	 * untouched
	 */
	{
		auto entryNew = model.getFunction(0);
		QCOMPARE( entryNew, entryOld );
	}
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.getFunction( model.size() )
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
	// assertExpected( model, testData );
}

void TestModel::testFunctionReferences() {
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x-1", [](T x){ return C(x-1, 0); } },
		{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
	};
	initTestModel( &model, testData );
	assertAllFunctionsValid( model, testData );
	assertExpected( model, testData );
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
	assertExpected( model, expectedResult );
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

void TestModel::testValuesToAudioBuffer()
{
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x-1", [](T x){ return C(x-1, 0); } },
		{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
	};
	initTestModel( &model, testData );

	const T duration = 1;
	const T speed = 1;
	const T offset = 0;
	const unsigned int samplerate = 44100;
	for( uint iFunction=0; iFunction<testData.size(); iFunction++ ) {
		std::vector<float> buffer;
		model.valuesToAudioBuffer(
				iFunction,
				&buffer,
				duration,
				speed,
				offset,
				samplerate,
				[](auto){ return 1; }
		);
		auto testFunction = testData[iFunction].second;
		QCOMPARE( buffer.size(), 44100 );
		for( unsigned int i=0; i<buffer.size(); i++ ) {
			T time = T(i)/samplerate;
			T expected = testFunction( C(speed * time + offset, 0) );
			if( expected <= 1 && expected >= -1) {
				QVERIFY2(
						qFuzzyCompare( buffer[i], float(expected) ),
						QString("FAILED: for f(x) = '%1': buffer[%2] == %3 != %4")
							.arg( testData[iFunction].first )
							.arg( i )
							.arg( buffer[i] )
							.arg( expected )
						.toStdString().c_str()
				);
			}
			// if out of range [-1,1], check for clipping:
			else {
				QVERIFY2(
						buffer[i] >= -1 && buffer[i] < 1,
						QString("FAILED: for f(x) = '%1': buffer[%2] == %3 not in range [-1,1]")
							.arg( testData[iFunction].first )
							.arg( i )
							.arg( buffer[i] )
						.toStdString().c_str()
				);
			}
		}
	}
}

/* utilities */

void assertAllFunctionsValid(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
)
{
	for( uint i=0; i<expectedResult.size(); i++ ) {
		auto expectedString = expectedResult[i].first ;
		auto expectedFunc = expectedResult[i].second ;
		auto errOrFunc = model.getFunction( i );
		{
			const auto error = 
					!errOrFunc
					? errOrFunc.error()
					: ""
			;
			QVERIFY2(
					errOrFunc,
					QString("error in expression: %1")
						.arg( error )
					.toStdString().c_str()
			);
		}
	}
}

void assertExpected(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
)
{
	const std::pair<T,T> range = {-3, 3};
	for( uint i=0; i<expectedResult.size(); i++ ) {
		auto expectedString = expectedResult[i].first ;
		auto expectedFunc = expectedResult[i].second ;
		auto errOrFunc = model.getFunction( i );
		auto func = errOrFunc.value();
		QCOMPARE( func->toString(), expectedString );
		for( int x=range.first; x<range.second; x++ ) {
			C y = func->get( C(x,0) );
			C yExpected = expectedFunc(T(x));
			QVERIFY2(
					cmplx::equal( y, yExpected ),
					QString( "error in f%1(%2) == (%3,%4) != (%5,%6 (expected)" )
						.arg( i )
						.arg( T(x) )
						.arg( y.c_.real() )
						.arg( y.c_.imag() )
						.arg( yExpected.c_.real() )
						.arg( yExpected.c_.imag() )
					.toStdString().c_str()
			);
		}
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
			QCOMPARE( graph[0].second, y0 );
			QCOMPARE( graph[graph.size()-1].second, yMax );
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
			QVERIFY2(
					qFuzzyCompare( point.second.c_.real(), expectedY.c_.real() )
						&& qFuzzyCompare( point.second.c_.imag(), expectedY.c_.imag() )
					,
					QString( "error in graph(f%1): %2 -> %3 != %4 (expected)" )
						.arg( iFunction )
						.arg( to_qstring( point.first ) )
						.arg( to_qstring( point.second ) )
						.arg( to_qstring( expectedY ) )
					.toStdString().c_str()
			);
		}
	}
}
