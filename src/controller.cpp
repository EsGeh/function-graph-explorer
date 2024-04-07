#include "controller.h"
#include <algorithm>
#include <variant>


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
				jack->stop();
				const auto rampTime = T(20)/1000;
				model->valuesToAudioBuffer(
						i,
						jack->getSampleTable(),
						duration,
						speed,
						offset,
						jack->getSamplerate(),
						[duration,rampTime](const double time) {
							return std::min(1.0,
								time < rampTime ? time/rampTime : (
									(duration-time) < rampTime ? (duration-time)/rampTime : 1
								)
							);
						}
				);
				jack->play();
			}
		);
	}
}

void Controller::updateFormula(const size_t iFunction) {
	const auto functionView = view->getFunctionView(iFunction);
	functionView->setFormula(
		model->getFormula(iFunction)
	);

	MaybeError maybeError = model->getError( iFunction );
	maybeError.and_then([functionView](auto error) {
		functionView->setFormulaError( error );
		return MaybeError{};
	} );
}

void Controller::updateGraph(const size_t iFunction) {
	const auto functionView = view->getFunctionView(iFunction);
	// update formula:
	{
		auto maybeError = model->set( 
				iFunction,
				functionView->getFormula()
		);
		maybeError.and_then([functionView](auto error) {
			functionView->setFormulaError( error );
			return MaybeError{};
		} );
	}
	// update graph:
	{
		auto errorOrPoints = model->getGraph(
				iFunction,
				functionView->getViewData().getXRange()
		);
		if( errorOrPoints ) {
			auto points = errorOrPoints.value();
			functionView->setGraph( points );
		}
	}
}

void Controller::run() {
	// run:
	view->show();
}
