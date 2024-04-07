#include "modelbenchmark.h"

#include "testutils.h"
#include <qtestcase.h>

QTEST_MAIN(ModelBenchmark)
#include "modelbenchmark.moc"
	

void ModelBenchmark::harmonicSeries_data()
{
	QTest::addColumn<unsigned int>("NumHarmonics");
	for( unsigned int N=0; N<=10; N+= 5 ) {
		QTest::newRow( QString::number(N).toStdString().c_str() ) << N;
	}
}

void ModelBenchmark::harmonicSeriesChain_data()
{
	QTest::addColumn<unsigned int>("NumHarmonics");
	for( unsigned int N=0; N<=10; N+= 5 ) {
		QTest::newRow( QString::number(N).toStdString().c_str() ) << N;
	}
}

void ModelBenchmark::harmonicSeries()
{
	QFETCH( unsigned int, NumHarmonics );
	Model model;
	std::vector<QString> testData = {
		(QStringList {
			"var freq := 440;",
			"var N := %1;",
			"var acc := 0;",
			"for( var k:=1; k<=N; k+=1 ) {",
			"  acc += cos( k*freq*2pi*x);",
			"};",
			"1/N*acc;"
		}).join("\n")
			.arg( NumHarmonics )
	};
	initTestModel( &model, testData );
	QBENCHMARK(
			model.getGraph(0, {0,1})
	);
}

void ModelBenchmark::harmonicSeriesChain()
{
	QFETCH( unsigned int, NumHarmonics );
	Model model;
	std::vector<QString> testData = {
		(QStringList {
			"var freq := 440;",
			"var N := %1;",
			"var acc := 0;",
			"for( var k:=1; k<=N; k+=1 ) {",
			"  acc += cos( k*freq*2pi*x);",
			"};",
			"1/N*acc;"
		}).join("\n")
			.arg( 10 ),
		(QStringList {
			"var N := %1;",
			"var acc := 0;",
			"for( var k:=1; k<=N; k+=1 ) {",
			"  acc += f0( k*x);",
			"};",
			"1/N*acc;"
		}).join("\n")
			.arg( NumHarmonics )
	};
	initTestModel( &model, testData );
	QBENCHMARK(
			model.getGraph(1, {0,1})
	);
}
