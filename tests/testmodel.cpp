#include "testmodel.h"
#include "model/model.h"
#include <memory>
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
	for( auto i=0; i<model.size(); i++ ) {
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
					!errOrFunc.index()
					? std::get<QString>(errOrFunc)
					: ""
			;
			QVERIFY2(
					errOrFunc.index() == 1,
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
	assertExpected( model, testData );
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
	model.set( 0, "x+1" );
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
	for( auto iFunction=0; iFunction<testData.size(); iFunction++ ) {
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
	for( auto i=0; i<expectedResult.size(); i++ ) {
		auto expectedString = expectedResult[i].first ;
		auto expectedFunc = expectedResult[i].second ;
		auto errOrFunc = model.getFunction( i );
		{
			const auto error = 
					!errOrFunc.index()
					? std::get<QString>(errOrFunc)
					: ""
			;
			QVERIFY2(
					errOrFunc.index() == 1,
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
	for( auto i=0; i<expectedResult.size(); i++ ) {
		auto expectedString = expectedResult[i].first ;
		auto expectedFunc = expectedResult[i].second ;
		auto errOrFunc = model.getFunction( i );
		auto func =
			std::get<std::shared_ptr<Function>>(errOrFunc)
		;
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
	const unsigned int resolution = 1024;
	const std::pair<T,T> range = {-3, 3};
	for( auto i=0; i<expectedResult.size(); i++ ) {
		auto expectedString = expectedResult[i].first ;
		auto expectedFunc = expectedResult[i].second ;
		auto errOrGraph = model.getGraph(
				i,
				range
		);
		{
			const auto error = 
					!errOrGraph.index()
					? std::get<QString>(errOrGraph)
					: ""
			;
			QVERIFY2(
					errOrGraph.index() == 1,
					QString("error in expression: %1")
						.arg( error )
					.toStdString().c_str()
			);
		}
		auto graph =
			std::get<std::vector<std::pair<C,C>>>(errOrGraph)
		;
		QCOMPARE( graph.size(), resolution );
		QCOMPARE( graph[0].first, range.first );
		QCOMPARE( graph[resolution-1].first, range.second );
		std::vector<std::pair<C,C>> expectedGraph;
		for( int j=0; j<resolution; j++ ) {
			T xVal =
				range.first
				+ T(j)/(resolution-1) * (range.second - range.first);
			std::pair<C,C> expectedPoint = {
				C(xVal,0),
				C(expectedFunc(xVal),0)
			};
			auto point = graph[j];
			QVERIFY2(
					cmplx::equal( point.first, expectedPoint.first )
					&& cmplx::equal( point.second, expectedPoint.second )
					,
					QString( "error in graph(f%1)[%2] == %3 != %4 (expected)" )
						.arg( i )
						.arg( j )
						.arg( to_qstring( point ) )
						.arg( to_qstring( expectedPoint ) )
					.toStdString().c_str()
			);
		}
	}
}
