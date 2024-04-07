#include <QTest>

class ModelBenchmark: public QObject
{
	Q_OBJECT
private slots:
	void harmonicSeries_data();
	void harmonicSeriesChain_data();

	void harmonicSeries();
	void harmonicSeriesChain();
};
