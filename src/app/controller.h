#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "fge/model/model.h"
#include "fge/view/mainwindow.h"
#include "fge/view/jack.h"

#include <QObject>
#include <cstddef>

class Controller : public QObject
{
	Q_OBJECT

public:
	Controller(
		Model* model,
		MainWindow* view,
		JackClient* jack
	);
	void run();

private:
	void setFunctionCount(const size_t size);

	// view.formula <- model.formula:
	void updateFormula(const size_t iFunction);

	// view.graph <- model.formula:
	void updateGraph( const size_t iFunction);

private:
	Model* model;
	MainWindow* view;
	JackClient* jack;

};

#endif // CONTROLLER_H
