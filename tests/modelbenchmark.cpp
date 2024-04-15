#include "modelbenchmark.h"

#include "testutils.h"
#include <qtestcase.h>

QTEST_MAIN(ModelBenchmark)
#include "modelbenchmark.moc"

const uint sampleResolution = 44100;

void ModelBenchmark::testData() {
	QTest::addColumn<unsigned int>("cacheResolution");
	QTest::addColumn<bool>("interpolation");
	QTest::addColumn<unsigned int>("numHarmonics");
	for( uint N=0; N<=10; N+= 10 ) {
		for( uint cacheResolution : {uint(0), sampleResolution} ) {
			auto interpol_values = (cacheResolution!=0) ? std::vector({false, true}) : std::vector({false});
			for( bool interpolation : interpol_values ) {
				QTest::addRow(
						"cacheResolution=%d, interpolation=%d, N=%d",
						cacheResolution,
						interpolation,
						N
				)
					<< cacheResolution
					<< interpolation
					<< N;
			}
		}
	}
}

void ModelBenchmark::harmonicSeries_data()
{
	testData();
}

void ModelBenchmark::harmonicSeriesChain_data()
{
	testData();
}

void ModelBenchmark::harmonicSeries()
{
	QFETCH( unsigned int, cacheResolution );
	QFETCH( bool, interpolation );
	QFETCH( unsigned int, numHarmonics );
	Model model(cacheResolution, interpolation);
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
				sampleResolution
			)
	);
}

void ModelBenchmark::harmonicSeriesChain()
{
	QFETCH( unsigned int, cacheResolution );
	QFETCH( bool, interpolation );
	QFETCH( unsigned int, numHarmonics );
	Model model(cacheResolution, interpolation);
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
				sampleResolution
			)
	);
}
