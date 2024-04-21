#include <QTest>


class ModelBenchmark: public QObject
{
	Q_OBJECT

void testData();
private slots:
	void harmonicSeries_data();
	void harmonicSeriesChain_data();

	void harmonicSeries();
	void harmonicSeriesChain();
};
