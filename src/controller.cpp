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
		[this]( size_t value ) {
			setFunctionCount( value );
		}
	);
	setFunctionCount( view->getFunctionViewCount() );
}

void Controller::setFunctionCount(const size_t size) {
	auto oldSize = model->size();
	model->resize( size );
	view->resizeFunctionView( size );
	for( size_t i=oldSize; i<model->size(); i++ ) {
		updateFormula(i);
		updateGraph(i);
		auto functionView = view->getFunctionView(i);
		// Model -> View:
		connect(
			functionView,
			&FunctionView::formulaChanged,
			[this,i]() {
				/* all graphs from the current
				 * starting from current index
				 * need to be repainted:
				 */
				for( auto j=i; j<model->size(); j++ ) {
					updateGraph(j);
				}
			}
		);
		connect(
			functionView,
			&FunctionView::viewParamsChanged,
			[this,i]() {
				updateGraph(i);
			}
		);
	}
}

void Controller::updateFormula(const size_t iFunction) {
	const auto errorOrFormula = model->get(iFunction);
	const auto functionView = view->getFunctionView(iFunction);
	if( errorOrFormula.index() == 0 ) {
		functionView->setFormulaError(
				std::get<QString>( errorOrFormula )
		);
	}
	else {
		functionView->setFormula( std::get<std::shared_ptr<Function>>(errorOrFormula)->toString() );
	}
}

void Controller::updateGraph(const size_t iFunction) {
	const auto functionView = view->getFunctionView(iFunction);
	auto errorOrFormula = model->set( 
			iFunction,
			functionView->getFormula()
	);
	if( errorOrFormula.index() == 0 ) {
		functionView->setFormulaError( std::get<QString>(errorOrFormula) );
		return;
	}
	functionView->setGraph(
			std::get<std::shared_ptr<Function>>(errorOrFormula)->getPoints(
				functionView->getGraphView()->getXRange()
			)
	);
}

void Controller::run() {
	// run:
	view->show();
}
