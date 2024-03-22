#include "controller.h"


const unsigned int RES = 100;

Controller::Controller(
	Model* model,
	MainWindow* view
):
	model(model),
	view(view)
{
	connect(
		view,
		&MainWindow::updateGraph,
		[=]() {
			model->set(
					view->getFormula()
			);
			view->setGraph(
					model->getPoints(
						view->getGraphView()->getXRange()
					)
			);
		}
	);
}

void Controller::run() {
	const auto formula = "sin(2pi*x)";
	view->setFormula( formula );
	model->set( formula );
	view->show();
}
