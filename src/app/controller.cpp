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
				auto maybeError = jack->start(
						Callbacks{
							// audio callback:
							.valuesToBuffer = [model](std::vector<float>* buffer, const PlaybackPosition position, const uint samplerate) {
								model->valuesToBuffer( buffer, position, samplerate );
							},
							// update:
							.betweenAudioCallback = [model](auto position, auto samplerate) {
								model->betweenAudio( position, samplerate );
							}
						}
				);
				if( !maybeError ) {
					model->setAudioSchedulingEnabled(true);
				}
			}
			else {
				model->setAudioSchedulingEnabled(false);
				jack->stop();
			}
		}
	);
	setFunctionCount( view->getFunctionViewCount() );
}

void Controller::setFunctionCount(const size_t size) {
	auto oldSize = model->size();
	model->resize( size );
	model->postSetAny();
	view->resizeFunctionView( size );
	for( size_t i=oldSize; i<model->size(); i++ ) {
		updateFormula(i);
		updateGraph(i);
		auto functionView = view->getFunctionView(i);
		functionView->setSamplingSettings(
				model->getSamplingSettings(i)
		);
		connect(
			functionView,
			&FunctionView::changed,
			[this,i,functionView](auto updateInfo) {
				auto maybeError = model->bulkUpdate(i,Model::Update{
						.formula = updateInfo.formula,
						.parameters = updateInfo.parameters,
						.stateDescriptions = updateInfo.stateDescriptions,
						.playbackEnabled = updateInfo.playbackEnabled,
						.samplingSettings = updateInfo.samplingSettings
				});
				maybeError.and_then([functionView](auto error) {
					functionView->setFormulaError( error );
					return MaybeError{};
				} );
				// update Graph:
				if(
						updateInfo.formula.has_value()
						|| updateInfo.parameters.has_value()
						|| updateInfo.stateDescriptions.has_value()
						|| updateInfo.samplingSettings.has_value()
				) {
					for( auto j=i; j<model->size(); j++ ) {
						updateGraph(j);
					}
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
	const auto functionView = view->getFunctionView(iFunction);
	functionView->setFormula(
		model->get(iFunction).formula
	);

	MaybeError maybeError = model->getError( iFunction );
	maybeError.and_then([functionView](auto error) {
		functionView->setFormulaError( error );
		return MaybeError{};
	} );
}

void Controller::updateGraph(const size_t iFunction) {
	const auto functionView = view->getFunctionView(iFunction);
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

void Controller::run() {
	// run:
	view->show();
}
