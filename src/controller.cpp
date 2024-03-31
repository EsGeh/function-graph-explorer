#include "controller.h"
#include <algorithm>


Controller::Controller(
	Model* model,
	MainWindow* view,
	JackClient* jack
)
	: model(model)
	, view(view)
	, jack(jack)
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
		connect(
			functionView,
			&FunctionView::playButtonPressed,
			[this,i](
					const T duration,
					const T speed,
					const T offset
			) {
				auto errorOrFormula = model->get(i);
				if( errorOrFormula.index() == 0 ) {
					return;
				}
				auto function =
					std::get<std::shared_ptr<Function>>(errorOrFormula);
				jack->stop();
				const unsigned int countSamples = duration*jack->getSamplerate(); 
				const auto rampTime = T(20)/1000;
				jack->getSampleTable()->resize( countSamples );
				for( unsigned int i=0; i<countSamples; i++ ) {
					T time = T(i)/jack->getSamplerate();
					// call function:
					T y = function->get( C(speed * time + offset, 0) );
					// ramp in and out
					// to prevent "clicking":
					T vol = std::min(1.0,
							time < rampTime ? time/rampTime : (
								(duration-time) < rampTime ? (duration-time)/rampTime : 1
							)
						);
					y = y * vol;
 					// clip audio level [-1,1]
					y = std::clamp( y, -1.0, +1.0 );
					jack->getSampleTable()->at(i) = y;
				}
				jack->play();
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
