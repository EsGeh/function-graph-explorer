#pragma once

#include <QTest>

class TestFormulaFunction: public QObject
{
	Q_OBJECT
private slots:
	void testInit_data();
	void testInit();
	void testEval();
	void testResolution_data();
	void testResolution();
};
