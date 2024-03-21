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
		&MainWindow::formulaChanged,
		[=](QString value) {
			// qDebug() << "Formula changed";
			model->set( value );
			updateGraph();
		}
	);
	connect(
		view,
		&MainWindow::xMinChanged,
		[=](T value) {
			// qDebug() << "xMin changed";
			model->setXMin( value );
			updateGraph();
		}
	);

	connect(
		view,
		&MainWindow::xMaxChanged,
		[=](T value) {
			// qDebug() << "xMax changed";
			model->setXMax( value );
			updateGraph();
		}
	);
	connect(
		view,
		&MainWindow::xMinReset,
		[=]() {
			// qDebug() << "xMin reset";
			model->resetXMin();
			view->setXRange( model->getXMin(), model->getXMax() );
			updateGraph();
		}
	);
	connect(
		view,
		&MainWindow::xMaxReset,
		[=]() {
			//qDebug() << "xMax reset";
			model->resetXMax();
			view->setXRange( model->getXMin(), model->getXMax() );
			updateGraph();
		}
	);

}

void Controller::run() {
	view->setFormula( "x^2" );
	model->set( "x^2" );
	view->setXRange( model->getXMin(), model->getXMax() );
	updateGraph();
	view->show();
}

void Controller::updateGraph() {
	if( !model->getIsValidExpression() ) {
		view->setFormulaError( "invalid formula" );
		return;
	}
	auto graph = model->getPoints();
	view->setGraph( graph );
}
