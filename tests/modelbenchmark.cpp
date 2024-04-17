#include "modelbenchmark.h"

#include "testutils.h"
#include <qtestcase.h>

QTEST_MAIN(ModelBenchmark)
#include "modelbenchmark.moc"

const uint sampleResolution = 44100;

void ModelBenchmark::testData() {
	QTest::addColumn<uint>("resolution");
	QTest::addColumn<uint>("interpolation");
	QTest::addColumn<uint>("numHarmonics");
	for( uint N=0; N<=10; N+= 10 ) {
		for( uint resolution : {uint(0), sampleResolution} ) {
			auto interpol_values = (resolution!=0) ? std::vector({0, 1}) : std::vector({0});
			for( uint interpolation : interpol_values ) {
				QTest::addRow(
						"resolution=%d, interpolation=%d, N=%d",
						resolution,
						interpolation,
						N
				)
					<< resolution
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
	QFETCH( uint, resolution );
	QFETCH( uint, interpolation );
	QFETCH( uint, numHarmonics );
	SamplingSettings settings{
		.resolution = resolution,
		.interpolation = interpolation,
		.caching = true
	};
	Model model(settings);
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
	QFETCH( uint, resolution );
	QFETCH( uint, interpolation );
	QFETCH( uint, numHarmonics );
	SamplingSettings settings{
		.resolution = resolution,
		.interpolation = interpolation,
		.caching = true
	};
	Model model(settings);
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
