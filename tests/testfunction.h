#include <QTest>

class TestFormulaFunction: public QObject
{
	Q_OBJECT
private slots:
	void testInit_data();
	void testInit();
	void testEval();
};