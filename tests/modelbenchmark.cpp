#include "modelbenchmark.h"

#include "testutils.h"
#include <qtestcase.h>

QTEST_MAIN(ModelBenchmark)
#include "modelbenchmark.moc"
	

void ModelBenchmark::harmonicSeries_data()
{
	QTest::addColumn<unsigned int>("resolution");
	QTest::addColumn<unsigned int>("numHarmonics");
	for( unsigned int resolution : {1024, 44100} ) {
		for( unsigned int N=0; N<=10; N+= 5 ) {
			QTest::addRow( "res=%d, N=%d", resolution, N )
				<< resolution
				<< N;
		}
	}
}

void ModelBenchmark::harmonicSeriesChain_data()
{
	QTest::addColumn<unsigned int>("resolution");
	QTest::addColumn<unsigned int>("numHarmonics");
	for( unsigned int resolution : {1024, 44100} ) {
		for( unsigned int N=0; N<=10; N+= 5 ) {
			QTest::addRow( "res=%d, N=%d", resolution, N )
				<< resolution
				<< N;
		}
	}
}

void ModelBenchmark::harmonicSeries()
{
	QFETCH( unsigned int, resolution );
	QFETCH( unsigned int, numHarmonics );
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
			.arg( numHarmonics )
	};
	initTestModel( &model, testData );
	QBENCHMARK(
			model.getGraph(
				0,
				{0,1},
				resolution
			)
	);
}

void ModelBenchmark::harmonicSeriesChain()
{
	QFETCH( unsigned int, resolution );
	QFETCH( unsigned int, numHarmonics );
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
			.arg( numHarmonics )
	};
	initTestModel( &model, testData );
	QBENCHMARK(
			model.getGraph(
				1,
				{0,1},
				resolution
			)
	);
}
