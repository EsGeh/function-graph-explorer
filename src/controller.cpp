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

	connect(
		view,
		&MainWindow::zoom,
		[=]( int value ) {
			const auto range = model->getXMax() - model->getXMin();
			if( value > 0 ) {
				view->setXRange( model->getXMin()-range/2, model->getXMax()+range/2 );
			}
			else {
				view->setXRange(
						std::min(T(-1.0/pow(2,4)), model->getXMin()+range/4 ),
						std::max(T(+1.0/pow(2,4)), model->getXMax()-range/4 )
				);
			}
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
