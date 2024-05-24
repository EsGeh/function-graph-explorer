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
	: model(model)
	, view(view)
	, jack(jack)
	, viewResolution(viewResolution)
{
	// modelWorker:
	workerThread = new QThread(this);
	modelWorker = new ModelWorker( model );
	modelWorker->moveToThread( workerThread );
	workerThread->start();
	connect(workerThread, &QThread::finished, modelWorker, &QObject::deleteLater);

	// resize: view -> modelWorker
	connect(
		view,
		&MainWindow::functionCountChanged,
		modelWorker,
		[this]( uint value ) {
			modelWorker->resize( value );
			// resizeView( value );
		}
	);
	// resize: view <- modelWorker
	connect(
		modelWorker,
		&ModelWorker::resizeDone,
		this,
		[this]( const uint size, const uint oldSize ) {
			resizeView( size, oldSize );
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
	// set initial model size:
	modelWorker->resize(
			view->getFunctionViewCount()
	);
}

Controller::~Controller()
{
	workerThread->quit();
	workerThread->wait();
}

void Controller::resizeView(
		const uint size,
		const uint oldSize
) {
	view->resizeFunctionView( size );
	for( uint index=oldSize; index<model->size(); index++ ) {
		setViewFormula(index);
		setViewGraph(index);
		auto functionView = view->getFunctionView(index);
		functionView->setSamplingSettings(
				model->getSamplingSettings(index)
		);
		// view -> modelWorker
		//           |
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
								.stateDescriptions = updateInfo.stateDescriptions,
								.playbackEnabled = updateInfo.playbackEnabled,
								.samplingSettings = updateInfo.samplingSettings
							}
					);
				}
		);
		//           |
		// view <- modelWorker:
		connect(
				modelWorker,
				&ModelWorker::updateDone,
				this,
				[this,functionView](auto index, auto updateInfo, auto maybeError) {
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
							setViewGraph(j);
						}
					}
				}
		);
		connect(
			functionView,
			&FunctionView::viewParamsChanged,
			[this,index]() {
				setViewGraph(index);
			}
		);
	}
}

void Controller::setViewFormula(const uint iFunction) {
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

void Controller::setViewGraph(const uint iFunction) {
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
