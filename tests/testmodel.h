#include <QTest>

class TestModel: public QObject
{
	Q_OBJECT
private slots:
	void testInit();
	void testResizeUp();
	void testResizeDown();
	void testSetEntry();
	void testFunctionReferences();
	void testUpdatesReferences();
};
