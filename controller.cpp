#include "controller.h"


const T MIN = -1;
const T MAX = 1;
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
		[=](QString val) {
			// qDebug() << "changed";
			try {
				model->set(val);
				std::vector<std::pair<T,T>> graph;
				for( int i=0; i<RES+1; i++ ) {
					T x = MIN + (T(i) / RES)*(MAX - MIN);
					graph.push_back(
							{
								x,
								model->get( x ),
							}
					);
				}
				view->set_graph( graph );
			}
			catch( ... ) {
				view->set_formula_error( "invalid formula" );
			}
		}
	);

}

void Controller::run() {
	view->init();
	view->show();
}
