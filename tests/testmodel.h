#ifndef TESTMODEL_H
#define TESTMODEL_H

#include "model/model.h"
#include "testutils.h"
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
	void testGetGraph();
	void testValuesToAudioBuffer();
};

/* utilities */

void assertAllFunctionsValid(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
);

void assertExpected(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
);

void assertCorrectGraph(
		const Model& model,
		const std::vector<std::pair<QString, std::function<C(T)>>>& expectedResult
);

#endif
