#include "model/model.h"

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
	// TODO: test Model::getGraph
	// TODO: test Model::valuesToAudioBuffer
};

void assertExpected(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
);
