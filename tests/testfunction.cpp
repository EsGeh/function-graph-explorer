#include <cstdlib>
#include <qtestcase.h>
#include <stdexcept>
#include "testfunction.h"
#include "fge/model/function.h"

QTEST_MAIN(TestFormulaFunction)
#include "testfunction.moc"

struct TestFunc:
	public exprtk::ifunction<C> 
{
	TestFunc()
		: exprtk::ifunction<C>(1)
	{}
	C operator()(const C& x) {
		return x;
	}
};

void TestFormulaFunction::testInit_data() {
	QTest::addColumn<QString>("formulaString");
	QTest::addColumn<ParameterBindings>("parameters");
	QTest::addColumn<Symbols>("symbols");
	QTest::addColumn<bool>("result");

	QTest::newRow("simple")
		<< "x"
		<< ParameterBindings{}
		<< Symbols()
		<< true
	;
	QTest::newRow("with parameter t")
		<< "x * t"
		<< ParameterBindings{ {"t", C(0,0) } }
		<< Symbols()
		<< true
	;
	QTest::newRow("unspecified parameter t fails")
		<< "x * t"
		<< ParameterBindings{}
		<< Symbols()
		<< false
	;
	QTest::newRow("x^2")
		<< "x^2"
		<< ParameterBindings{}
		<< Symbols()
 		<< true
	;
	// const from symbol table:
	{
		QTest::newRow("const from symbol table")
			<< "2pi*x"
			<< ParameterBindings{}
			<< Symbols({ {"pi", C(3.141, 0)} })
			<< true
		;
	}
	QTest::newRow("unknown var")
		<< "2pi*x"
		<< ParameterBindings{}
		<< Symbols()
		<< false
	;
	// function from symbol table:
	{
		static TestFunc func;
		QTest::newRow("function from symbol table")
			<< "f1(2*x)"
			<< ParameterBindings{}
			<< Symbols({}, { { "f1", &func } })
			<< true
		;
	}
}

void TestFormulaFunction::testInit() {
	// fetch test args:
	QFETCH(QString, formulaString);
	QFETCH(ParameterBindings, parameters);
	QFETCH(Symbols, symbols);
	QFETCH(bool, result);
	// run test:
	auto errOrValue = formulaFunctionFactory(
			formulaString,
			parameters,
			{symbols},
			0 // no caching
	);
	if( result == 0 ) {
		QVERIFY( !errOrValue );
	}
	else {
		QVERIFY2(
			errOrValue,
			QString("failed with '%1'")
				.arg( !errOrValue ? errOrValue.error() : "" )
			.toStdString().c_str()
		);
	}
}

void checkFunction(
		Function* function,
		std::function<C(T)> expectedFunc,
		const std::pair<T,T>& range,
		const uint resolution
);

void TestFormulaFunction::testEval()
{
	auto errOrValue = formulaFunctionFactory(
			"x^2",
			{},
			{},
			0, // no quantization
			0, // no interpolation
			false // no caching
	);
	assert( errOrValue );
	auto function = errOrValue.value();
	checkFunction(
			function.get(),
			[](auto x){ return C(pow(x,2),0); },
			{-3, 3},
			44100
	);
}

void TestFormulaFunction::testEvalWithParameters()
{
	auto errOrValue = formulaFunctionFactory(
			"t * x^2",
			{ {"t", C(2,0)} },
			{},
			0, // no quantization
			0, // no interpolation
			false // no caching
	);
	assert( errOrValue );
	auto function = errOrValue.value();
	checkFunction(
			function.get(),
			[](auto x){ return C(2 * pow(x,2),0); },
			{-3, 3},
			8
	);
}

void TestFormulaFunction::testResolution_data() {
	QTest::addColumn<uint>("interpolation");
	QTest::addColumn<uint>("resolution");
	for( uint interpolation : { 0,1 } ) {
		// test for resolutions 1/( 2^ipow )
		for( uint ipow = 0; (1<<ipow)<10000; ipow++ ) {
			const uint resolution = (1<<ipow);
			QTest::addRow(
					"interpolation: %d, resolution: 2^%d = %d",
					interpolation,
					ipow,
					resolution
			) << interpolation << resolution;
		}
		// test typical audio resolutions:
		{
			const uint resolution = 44100;
			QTest::addRow(
					"interpolation: %d, resolution: %d",
					interpolation,
					resolution
			) << interpolation << resolution;
		}
	}
}

const T epsilon = 1.0/(1<<16);

bool float_equal(const T f1, const T f2) {
	return std::abs(f2-f1) < epsilon;
}

void TestFormulaFunction::testResolution()
{
	QFETCH(uint, interpolation);
	QFETCH(uint, resolution);
	const std::pair<T,T>& range{ -3, 3 };
	const uint subRes = 10;
	const QString formula = "x^2";
	auto function = formulaFunctionFactory(
			formula,
			{},
			{},
			0, // no quantization
			0,
			false // no caching
	).value();
	auto quantizedFunction = formulaFunctionFactory(
			formula,
			{},
			{},
			resolution, // quantization
			interpolation,
			false // no caching
	).value();
	const T rangeDelta = range.second - range.first;
	for( int i=0; i<int(resolution * rangeDelta)+1; i++ )
	{
		const double x = range.first + T(i) / resolution;
		const C y = function->get( C(x,0) );
		const double xNext = range.first + T(i+1) / resolution;
		const C yNext = function->get( C(xNext,0) );
		// check values falling on full steps:
		{
			const C ret = quantizedFunction->get( C(x,0) );
			QVERIFY2(
					float_equal( ret.c_.real(), y.c_.real() ),
					QString( "error in function: %1 -> %2 != %3 (expected)" )
						.arg( to_qstring( C(x,0) ) )
						.arg( to_qstring( ret ) )
						.arg( to_qstring( y ) )
					.toStdString().c_str()
			);
		}
		// check values BETWEEN full steps:
		for(uint isub=1; isub<subRes; isub++)
		{
			const T frac = T(isub) / T(subRes) / T(resolution);
			const T shiftedX = x+frac;
			const C ret = quantizedFunction->get( C(shiftedX,0) );
			if( interpolation == 0 ) {
					QVERIFY2(
							float_equal( ret.c_.real(), y.c_.real() ),
							QString( "error in function: %1 + %2 -> %3 != %4 (expected)" )
								.arg( to_qstring( C(x,0) ) )
								.arg( frac )
								.arg( to_qstring( ret ) )
								.arg( to_qstring( y ) )
							.toStdString().c_str()
					);
			}
			else {
				QVERIFY2(
						std::min(y.c_.real(), yNext.c_.real())
							<= ret.c_.real()
							&& std::max(y.c_.real(), yNext.c_.real())
							>= ret.c_.real()
						,
						QString( "error in function interpolaton: %1 + %2 -> %3 not between (%4, %5)  (expected)" )
							.arg( to_qstring( C(x,0) ) )
							.arg( frac )
							.arg( to_qstring( ret ) )
							.arg( to_qstring( y ) )
							.arg( to_qstring( yNext ) )
						.toStdString().c_str()
				);
			}
		}
	}
}

void checkFunction(
		Function* function,
		std::function<C(T)> expectedFunc,
		const std::pair<T,T>& range,
		const uint resolution
)
{
	const T rangeDelta = range.second - range.first;
	for( int i=0; i<int(resolution * rangeDelta)+1; i++ )
	{
		const double x = range.first + T(i) / resolution;
		const auto ret = function->get( C(x,0) );
		const T expectedY = expectedFunc(x);
		QVERIFY2(
				qFuzzyCompare( ret.c_.real(), expectedY )
				// && qFuzzyCompare( ret.c_.imag(), 0 )
				,
				QString( "error in function: %1 -> %2 != %3 (expected)" )
					.arg( to_qstring( C(x,0) ) )
					.arg( to_qstring( ret ) )
					.arg( to_qstring( C(expectedY,0) ) )
				.toStdString().c_str()
		);
	}
}
