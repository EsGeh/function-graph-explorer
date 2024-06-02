#include "controller.h"
#include <mutex>
#include <scoped_allocator>
#include <QtConcurrent>


Controller::Controller(
	Model* model,
	MainWindow* view,
	JackClient* jack,
	const uint viewResolution
)
	: view(view)
	, jack(jack)
	, viewResolution(viewResolution)
{

	// INITIALIZE:

	// modelWorker:
	workerThread = new QThread(this);
	modelWorker = new ModelWorker( model );
	modelWorker->moveToThread( workerThread );
	workerThread->start();
	connect(workerThread, &QThread::finished, modelWorker, &QObject::deleteLater);

	// CONNECTIONS:

	// resize: view -> modelWorker
	connect(
		view,
		&MainWindow::functionCountChanged,
		modelWorker,
		[this]( uint value ) {
			modelWorker->resize( value );
		}
	);
	// resize: view <- modelWorker
	connect(
		modelWorker,
		&ModelWorker::resizeDone,
		this,
		[this]( const Model* model, const uint oldSize ) {
			resizeView( model, oldSize );
			modelWorker->unlockModel();
		}
	);
	// view <- modelWorker:
	connect(
			modelWorker,
			&ModelWorker::updateDone,
			this,
			[this,view](auto model, auto index, auto updateInfo, auto maybeError) {
				auto functionView = view->getFunctionView(index);
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
					for( auto j=index; j<model->size(); j++ ) {
						setViewGraph(model, j);
					}
				}
				modelWorker->unlockModel();
			}
	);
	connect(
		modelWorker,
		&ModelWorker::readAccessGranted,
		this,
		[this](auto model, auto index) {
			setViewGraph(model, index);
			modelWorker->unlockModel();
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

	// SET INITIAL model size:

	modelWorker->resize(
			view->getFunctionViewCount()
	);
	modelWorker->unlockModel();

}

Controller::~Controller()
{
	workerThread->quit();
	workerThread->wait();
}

void Controller::resizeView(
		const Model* model,
		const uint oldSize
) {
	view->resizeFunctionView( model->size() );
	// initialize and connect new entries:
	for( uint index=oldSize; index<model->size(); index++ ) {
		// INITIALIZE:
		setViewFormula(model, index);
		setViewGraph(model, index);
		auto functionView = view->getFunctionView(index);
		functionView->setSamplingSettings(
				model->getSamplingSettings(index)
		);
		// CONNECT:
		// view -> modelWorker
		connect(
				functionView,
				&FunctionView::changed,
				modelWorker,
				[this,index](auto updateInfo) {
					return modelWorker->updateFunction(
							index,
							Model::Update{
								.formula = updateInfo.formula,
								.parameters = updateInfo.parameters,
								.parameterDescriptions = updateInfo.parameterDescriptions,
								.stateDescriptions = updateInfo.stateDescriptions,
								.playbackEnabled = updateInfo.playbackEnabled,
								.samplingSettings = updateInfo.samplingSettings
							}
					);
				}
		);
		connect(
				functionView,
				&FunctionView::parameterChanged,
				modelWorker,
				[this,index](auto parameterName, auto value) {
					return modelWorker->updateParameters(
							index,
							{ { parameterName, value } }
					);
				}
		);
		// view -> modelWorker
		connect(
			functionView,
			&FunctionView::viewParamsChanged,
			modelWorker,
			[this,index]() {
				modelWorker->requestRead( index );
			}
		);
	}
}

void Controller::setViewFormula(
		const Model* model,
		const uint iFunction
) {
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

void Controller::setViewGraph(const Model* model, const uint iFunction) {
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
