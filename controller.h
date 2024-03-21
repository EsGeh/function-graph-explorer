#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include "model.h"
#include "mainwindow.h"

class Controller : public QObject
{
	Q_OBJECT

public:
	Controller(
		Model* model,
		MainWindow* view
	);
	void run();

signals:
	void formulaChanged(QString formula);

private:
	Model* model;
	MainWindow* view;

};

#endif // CONTROLLER_H
