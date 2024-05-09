#ifndef TESTMODEL_H
#define TESTMODEL_H

#include "fge/model/model.h"

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
	void testValuesToBuffer();
};

#endif
