#include "controller.h"


Controller::Controller(
	Model* model,
	MainWindow* view,
	JackClient* jack,
	const uint viewResolution
)
	: model(model)
	, view(view)
	, jack(jack)
	, viewResolution(viewResolution)
{
	connect(
		view,
		&MainWindow::functionCountChanged,
		[this]( size_t value ) {
			setFunctionCount( value );
		}
	);
	connect(
		view,
		&MainWindow::isAudioEnabledChanged,
		[jack,model]( bool value ) {
			if(value) {
				jack->start([model](auto position, auto samplerate) {
						return model->audioFunction( position, samplerate );
				});
			}
			else {
				jack->stop();
			}
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
		functionView->setSamplingSettings(
				model->getSamplingSettings(i)
		);
		// Model -> View:
		connect(
			functionView,
			&FunctionView::formulaChanged,
			[this,i]() {
				/* all graphs from the current
				 * starting from current index
				 * need to be repainted:
				 */
				auto functionView = view->getFunctionView(i);
				MaybeError maybeError = model->setParameterValues(
						i,
						functionView->getParameters()
				);
				maybeError.and_then([functionView](auto error) {
					functionView->setFormulaError( error );
					return MaybeError{};
				} );
				model->setSamplingSettings(
						i,
						view->getFunctionView(i)->getSamplingSettings()
				);
				for( auto j=i; j<model->size(); j++ ) {
					updateGraph(j);
				}
			}
		);
		// View -> Model:
		connect(
			functionView,
			&FunctionView::viewParamsChanged,
			[this,i]() {
				updateGraph(i);
			}
		);
		connect(
			functionView,
			&FunctionView::playbackEnabledChanged,
			[this,i](bool value) {
				model->setIsPlaybackEnabled(i,value);
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
				functionView->getFormula(),
				functionView->getParameters(),
				functionView->getStateDescriptions()
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
				functionView->getViewData().getXRange(),
				viewResolution
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
