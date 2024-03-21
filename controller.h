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

private:

	void updateGraph();

private:
	Model* model;
	MainWindow* view;

};

#endif // CONTROLLER_H
