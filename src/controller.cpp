#include "controller.h"
#include <cstddef>


Controller::Controller(
	Model* model,
	MainWindow* view
):
	model(model),
	view(view)
{

	connect(
		view,
		&MainWindow::functionCountChanged,
		[=]( size_t value ) {
			setFunctionCount( value );
		}
	);
	setFunctionCount( view->getFunctionViewCount() );
}

void Controller::setFunctionCount(const size_t size) {
	const auto oldSize = model->size();
	if( oldSize > size ) {
		model->resize( size );
	}
	else if( oldSize < size ) {
		while( model->size() < size ) {
			const auto formulaStr = "sin(2pi*x)";
			auto formula = formulaFunctionFactory(
					formulaStr
			);
			model->push_back( formula );
		}
	}
	view->resizeFunctionView( size );
	for( size_t i=oldSize; i<model->size(); i++ ) {
		updateFormula(i);
		updateGraph(i);
		auto functionView = view->getFunctionView(i);
		// Model -> View:
		connect(
			functionView,
			&FunctionView::formulaChanged,
			[=]() {
				updateGraph(i);
			}
		);
		connect(
			functionView,
			&FunctionView::viewParamsChanged,
			[=]() {
				updateGraph(i);
			}
		);
	}
}

void Controller::updateFormula(const size_t iFunction) {
	const auto maybeFormula = model->at(iFunction);
	const auto functionView = view->getFunctionView(iFunction);
	if( !maybeFormula ) {
		functionView->setFormulaError("error initializing");
	}
	else {
		functionView->setFormula( maybeFormula.value()->toString() );
	}
}

void Controller::updateGraph(const size_t iFunction) {
	auto& maybeFormula = model->at(iFunction);
	const auto functionView = view->getFunctionView(iFunction);
	maybeFormula = formulaFunctionFactory(
			functionView->getFormula()
	);
	if( !maybeFormula ) {
		functionView->setFormulaError( "invalid function" );
		return;
	}
	functionView->setGraph(
			maybeFormula.value()->getPoints(
				functionView->getGraphView()->getXRange()
			)
	);
}

void Controller::run() {
	// run:
	view->show();
}
