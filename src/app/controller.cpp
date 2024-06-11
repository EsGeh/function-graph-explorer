#include "controller.h"
#include <mutex>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <scoped_allocator>
#include <QtConcurrent>

#ifdef LOG_CONTROLLER
#include <source_location>
#endif

#ifdef LOG_CONTROLLER
#define LOG_FUNCTION() \
	{ \
		const auto location = std::source_location::current(); \
		qDebug() << location.function_name(); \
	}
#else
#define LOG_FUNCTION()
#endif

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
	modelWorker = new ModelWorker( model, this );
	assert( view->getFunctionViewCount() == 1 );

	// CONNECTIONS:
	connect(
		modelWorker,
		&ModelWorker::writeDone,
		this,
		&Controller::writeDoneCallback
	);

	// resize:
	connect(
		view,
		&MainWindow::functionCountChanged,
		this,
		[this]( uint value ) {
			modelWorker->write( this, "resize",
					[value](auto model) {
						auto oldSize = model->size();
						model->resize( value );
						model->postSetAny();
						return oldSize;
					},
					[this](auto model, auto oldSize) {
						resizeView( model, oldSize );
					}
			);
		}
	);

	connect(
			view,
			&MainWindow::globalPlaybackSpeedChanged,
			[this]( auto value ) {
				modelWorker->write(this, "setPlaybackSpeed", [value](auto model){
						model->setPlaybackSpeed( value );
				},
				[](auto){});
			}
	);

	connect(
		view,
		&MainWindow::isAudioEnabledChanged,
		[this]( bool value ) {
			if(value) {
				startPlayback();
			}
			else {
				stopPlayback();
			}
		}
	);
	connect(
		modelWorker,
		&ModelWorker::tick,
		this,
		[this]() {
			modelWorker->read( this,
					[](auto model) {
						return double(model->getPosition()) / double(model->getSamplerate());
					},
					[view = this->view](auto model, auto pos){ view->setPlaybackTime( pos ); }
			);
		}
	);

	// SET INITIAL model size:
	modelWorker->write(
			this, "initialize model size",
			[view](auto model){
					model->resize( view->getFunctionViewCount() );
			},
			[this,view](auto model){
				resizeView( model, 0 );
				view->setGlobalPlaybackSpeed( model->getPlaybackSpeed() );
			}
	);
}

Controller::~Controller()
{
}

void Controller::run() {
	// run:
	view->show();
}

void Controller::exit()
{
	LOG_FUNCTION()
	stopPlayback().get();
	modelWorker->exit();
}

void Controller::writeDoneCallback(
		std::function<void()> doneCallback
)
{
	doneCallback();
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
		connect(
				functionView,
				&FunctionView::changed,
				this,
				[this,index](auto updateInfo) {
					modelWorker->write(
							this,
							"update",
							[index,updateInfo](auto model){
								return model->bulkUpdate(index,
									Model::Update{
										.formula = updateInfo.formula,
										.parameters = updateInfo.parameters,
										.parameterDescriptions = updateInfo.parameterDescriptions,
										.stateDescriptions = updateInfo.stateDescriptions,
										.playbackSettings = updateInfo.playbackSettings,
										.playbackEnabled = updateInfo.playbackEnabled,
										.samplingSettings = updateInfo.samplingSettings
									}
								);
							},
							[this,index,updateInfo](auto model, auto maybeError){
								auto functionView = view->getFunctionView(index);
								maybeError.and_then([&](auto error) {
									qDebug() << "ERROR" << index << error;
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
							}
					);
				}
		);
		connect(
				functionView,
				&FunctionView::parameterChanged,
				this,
				[this,index](auto parameterName, auto value) {
					modelWorker->write( this, "parameterChanged",
							[this,index, parameterName, value](auto model) -> MaybeError {
								// update parameters asynchronously:
								ParameterBindings parameters = { { parameterName, value } };
								ParameterBindings asyncParams = [&]{
									return model->scheduleSetParameterValues(
											index, parameters,
											[this](auto index, auto parameters) {
												modelWorker->write(this,"async parameter done",
														[](auto model) {
															model->postSetAny();
														},
														[this,index](auto model){
															// update Graph:
															for( auto j=index; j<model->size(); j++ ) {
																setViewGraph(model, j);
															}
														}
												);
											}
									);
								}();
								// the remaining parameters
								// are to be set synchronously:
								auto syncParams = parameters
									| std::ranges::views::filter([&asyncParams](auto entry){
										return !asyncParams.contains( entry.first );
									})
									| std::ranges::to<ParameterBindings>();
								if( syncParams.empty() ) {
									return {};
								}
								auto ret = model->setParameterValues( index, syncParams );
								model->postSetAny();
								return ret;
							},
							[this,index, parameterName, value](auto model, auto maybeError) {
								auto functionView = view->getFunctionView(index);
								maybeError.and_then([&](auto error) {
									qDebug() << "ERROR" << index << error;
									functionView->setFormulaError( error );
									return MaybeError{};
								} );
								// update Graph:
								for( auto j=index; j<model->size(); j++ ) {
									setViewGraph(model, j);
								}
							}
				);
			}
		);
		// view -> modelWorker
		connect(
			functionView,
			&FunctionView::viewParamsChanged,
			this,
			[this,index]{
				modelWorker->read(
						this,
						[](auto){},
						[this,index](auto model){
							setViewGraph(model, index);
						}
				);
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

void Controller::startPlayback()
{
	modelWorker->write( this, "setAudioSchedulingEnabled(true)",
			[jack = this->jack](auto model){
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
			},
			[this](auto){
				modelWorker->startTimer();
			}
	);
}

std::future<void> Controller::stopPlayback()
{
	std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
	auto future = promise->get_future();
	modelWorker->stopTimer();
	modelWorker->write( this, "setAudioSchedulingEnabled(false)",
			[this,promise](auto model){
				model->setAudioSchedulingEnabled(false);
				jack->stop();
				promise->set_value();
			},
			[](auto) {
			}
	);
	return future;
}
