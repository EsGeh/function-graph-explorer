#include "testmodel.h"
#include "model.h"
#include <exception>
#include <memory>
#include <shared_mutex>
#include <stdexcept>

QTEST_MAIN(TestModel)
#include "testmodel.moc"

void TestModel::testInit() {
	Model model;
	QCOMPARE( model.size(), 0 );
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.get( model.size() )
	);
}

void TestModel::testResizeUp() {
	Model model;
	model.resize(3);
	QCOMPARE( model.size(), 3 );
	for( auto i=0; i<model.size(); i++ ) {
		QVERIFY_THROWS_NO_EXCEPTION(
			model.get( i );
		);
		/* elements are initialized
		 * with a valid default
		 * function
		 */
		auto errOrFunc = model.get( i );
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
			/*
			QVERIFY_THROWS_NO_EXCEPTION(
				std::get<std::shared_ptr<Function>>(errorOrFunc)
			);
			*/
		}
	}
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.get( model.size() )
	);
}

void TestModel::testResizeDown() {
	Model model;
	model.resize(3);
	auto entryOld = model.get(0);
	model.resize(1);
	QCOMPARE( model.size(), 1 );
	/* downsizing invalidates
	 * entries from the tail,
	 * but leaves the head
	 * untouched
	 */
	{
		auto entryNew = model.get(0);
		QCOMPARE( entryNew, entryOld );
	}
	QVERIFY_THROWS_EXCEPTION(
			std::out_of_range,
			model.get( model.size() )
	);
}

void TestModel::testSetEntry() {
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x^1", [](auto x){ return C(x,0); } },
		{ "x^2", [](auto x){ return C(x*x,0); } },
		{ "x^3", [](auto x){ return C(x*x*x,0); } }
	};
	model.resize(testData.size());
	for( auto i=0; i<testData.size(); i++ ) {
		auto funcString = testData[i].first ;
		model.set(i, funcString );
	}
	for( auto i=0; i<testData.size(); i++ ) {
		auto expectedString = testData[i].first ;
		auto expectedFunc = testData[i].second ;
		auto errOrFunc = model.get( i ); /* all functions are valid
		 */
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
		/* all functions describe
		 * a calculation equal to
		 * the string from which
		 * it has been created:
		 */
		auto func =
			std::get<std::shared_ptr<Function>>(errOrFunc)
		;
		QCOMPARE( func->toString(), expectedString );
		for( int x=-3; x<3; x++ ) {
			C y = func->get( C(x,0) );
			C yExpected = expectedFunc(x);
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

void TestModel::testFunctionReferences() {
	Model model;
	std::vector<std::pair<QString, std::function<C(T)>>> testData = {
		{ "x-1", [](T x){ return C(x-1, 0); } },
		{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
	};
	model.resize(testData.size());
	for( auto i=0; i<testData.size(); i++ ) {
		auto funcString = testData[i].first ;
		QVERIFY_THROWS_NO_EXCEPTION(
			model.set(i, funcString );
		);
	}
	for( auto i=0; i<testData.size(); i++ ) {
		auto expectedString = testData[i].first ;
		auto expectedFunc = testData[i].second ;
		auto errOrFunc = model.get( i );
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
		auto func =
			std::get<std::shared_ptr<Function>>(errOrFunc)
		;
		QCOMPARE( func->toString(), expectedString );
		for( int x=-3; x<3; x++ ) {
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

void TestModel::testUpdatesReferences() {
	Model model;
	// initial state:
	{
		std::vector<std::pair<QString, std::function<C(T)>>> testData = {
			{ "x-1", [](T x){ return C(x-1, 0); } },
			{ "(x+1)*f0(x)", [](T x){ return C((x+1)*(x-1), 0); } }
		};
		/* set F0, F1: 
		 */
		model.resize(testData.size());
		for( auto i=0; i<testData.size(); i++ ) {
			auto funcString = testData[i].first ;
			QVERIFY_THROWS_NO_EXCEPTION(
				model.set(i, funcString );
			);
		}
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
	for( auto i=0; i<expectedResult.size(); i++ ) {
		auto expectedString = expectedResult[i].first ;
		auto expectedFunc = expectedResult[i].second ;
		auto errOrFunc = model.get( i );
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
		auto func =
			std::get<std::shared_ptr<Function>>(errOrFunc)
		;
		QCOMPARE( func->toString(), expectedString );
		for( int x=-3; x<3; x++ ) {
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
