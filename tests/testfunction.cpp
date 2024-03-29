#include <testfunction.h>
#include "function.h"

QTEST_MAIN(TestFormulaFunction)
#include "testfunction.moc"

typedef std::pair<QString,C> ConstDescr;
typedef std::pair<QString,QString> FuncDescr;;
typedef std::vector<ConstDescr> ConstDescrs;
typedef std::vector<FuncDescr> FuncDescrs;

void TestFormulaFunction::testInit_data() {
	QTest::addColumn<QString>("formulaString");
	QTest::addColumn<ConstDescrs>("constants");
	QTest::addColumn<FuncDescrs>("functions");
	QTest::addColumn<int>("result");

	QTest::newRow("simple")
		<< "x"
		<< ConstDescrs {}
		<< FuncDescrs {}
		<< 1
	;
	QTest::newRow("x^2")
		<< "x^2"
		<< ConstDescrs {}
		<< FuncDescrs {}
 		<< 1
	;
	// const from symbol table:
	{
		QTest::newRow("const from symbol table")
			<< "2pi*x"
			<< ConstDescrs { {"pi", C(3.141, 0)} }
			<< FuncDescrs {}
			<< 1
		;
	}
	QTest::newRow("unknown var")
		<< "2pi*x"
		<< ConstDescrs {}
		<< FuncDescrs {}
		<< 0
	;
	// function from symbol table:
	{
		QTest::newRow("function from symbol table")
			<< "f1(2*x)"
			<< ConstDescrs {}
			<< FuncDescrs { { "f1", "cos(x)" } }
			<< 1
		;
	}
}

void TestFormulaFunction::testInit() {
	// fetch test args:
	QFETCH(QString, formulaString);
	QFETCH(ConstDescrs, constants);
	QFETCH(FuncDescrs, functions);
	QFETCH(int, result);
	// run test:
	symbol_table_t symbols;
	std::vector<std::shared_ptr<compositor_t>> compositors;
	std::vector<symbol_table_t*> symbolTables;
	for( auto c : constants ) {
		symbols.add_constant(  c.first.toStdString().c_str(), c.second );
	}
	symbolTables.push_back( &symbols );
	for( auto f : functions ) {
		auto compositor = std::shared_ptr<compositor_t>( new compositor_t());
		compositor -> add(
				function_t()
				.name(f.first.toStdString().c_str())
				.var("x")
				.expression( f.second.toStdString().c_str() )
			);
		compositors.push_back( compositor );
		symbolTables.push_back(
				&(compositor->symbol_table())
		);
	}
	auto errOrValue = formulaFunctionFactory(
			formulaString,
			symbolTables
	);
	if( result == 0 ) {
		QCOMPARE( errOrValue.index(), result);
	}
	else {
		if( errOrValue.index() == result )
			QCOMPARE( errOrValue.index(), result);
		else {
			QVERIFY2(
				errOrValue.index() == result,
				(QString("failed with '") + std::get<QString>( errOrValue ) + "'"
				).toStdString().c_str()
			);
		}
	}
}

void TestFormulaFunction::testEval() {
	auto errOrValue = formulaFunctionFactory(
			"x^2",
			{}
	);
	assert( errOrValue.index() == 1 );
	auto function = std::get<std::shared_ptr<Function>>( errOrValue );
	{
		auto ret = function->get( C(0,0) );
		QCOMPARE( ret, C(0,0) );
	}
	{
		auto ret = function->get( C(2,0) );
		QCOMPARE( ret, C(4,0) );
	}
}
