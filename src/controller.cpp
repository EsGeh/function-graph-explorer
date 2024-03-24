#include "controller.h"
#include <cstddef>


Controller::Controller(
	Model* model,
	MainWindow* view
):
	model(model),
	view(view)
{
	updateFormula(0);
	updateGraph(0);
	// Model -> View:
	connect(
		view,
		&MainWindow::formulaChanged,
		[=]() {
			updateGraph(0);
		}
	);
	connect(
		view,
		&MainWindow::viewParamsChanged,
		[=]() {
			updateGraph(0);
		}
	);
}

void Controller::updateFormula(const size_t iFunction) {
	assert( iFunction < model->size() );
	const auto maybeFormula = model->at(iFunction);
	if( !maybeFormula ) {
		view->setFormulaError("error initializing");
	}
	else {
		view->setFormula( maybeFormula.value()->toString() );
	}
}

void Controller::updateGraph(const size_t iFunction) {
	assert( iFunction < model->size() );
	auto& maybeFormula = model->at(iFunction);
	maybeFormula = formulaFunctionFactory(
			view->getFormula()
	);
	if( !maybeFormula ) {
		view->setFormulaError( "invalid function" );
		return;
	}
	view->setGraph(
			maybeFormula.value()->getPoints(
				view->getGraphView()->getXRange()
			)
	);
}

void Controller::run() {
	// run:
	view->show();
}
