#include "fge/view/functionview.h"
#include "fge/view/keybindings.h"
#include "ui_functionview.h"
#include <qcheckbox.h>
#include <qnamespace.h>

FunctionView::FunctionView(
		const QString& title,
		const double* globalPlaybackSpeed,
		QWidget *parent
)
    : QWidget(parent)
    , ui(new Ui::FunctionView)
		, graphView(nullptr)
		, displayDialog(nullptr)
		, statusBar(nullptr)
		, viewData()
		, samplingSettings()
		, globalPlaybackSpeed( globalPlaybackSpeed )
{
	addFunctionViewKeys(this);
	ui->setupUi(this);
	ui->formulaLabel->setText( title );
	statusBar = new QStatusBar();
	statusBar->setVisible(false);
	ui->verticalLayout->addWidget( statusBar, 0 );
	graphView = new GraphView(
			&viewData
	);
	ui->parametersBtn->setVisible( parameters.size() > 0 );
	ui->verticalLayout->addWidget( graphView, 1 );
	setFocusProxy(
			ui->formulaEdit
	);

	parametersDialog = new ParametersEdit(
			"Function Parameters for " + title,
			&parameters,
			&dataDescription.parameterDescriptions,
			this
	);
	displayDialog = new FunctionDisplayOptions(
			viewData,
			samplingSettings,
			this
	);

	connect(
		ui->formulaEdit,
		&QLineEdit::textEdited,
		[this](QString value) {
			displayDialog->setFormula( value );
			emit changed({
					.formula = value
			});
		}
	);
	connect(
		ui->parametersBtn,
		&QAbstractButton::clicked,
		[this]() {
			parametersDialog->updateView();
			parametersDialog->show();
		}
	);
	connect(
		parametersDialog,
		&ParametersEdit::parameterChanged,
		[this](auto parameterName, auto value) {
			emit parameterChanged(
					parameterName, value
			);
		}
	);
	connect(
		ui->optionsBtn,
		&QAbstractButton::clicked,
		[this]() {
			openDisplayDialog();
		}
	);
	connect(
		ui->playbackEnabled,
		&QCheckBox::checkStateChanged,
		[this](auto value) {
			if( !value) {
				disablePlaybackPosition();
			}
			emit changed({
					.playbackEnabled = (value != 0)
			});
		}
	);
	connect(
		displayDialog,
		&FunctionDisplayOptions::finished,
		[this](int result) {
			if( !result ) return;
			dataDescription = parseFunctionDataDescription(
				displayDialog->getDataDescription()
			);
			updateParameters(
					dataDescription.parameterDescriptions,
					parameters
			);
			ui->parametersBtn->setVisible( parameters.size() > 0 );
			viewData = displayDialog->getViewData();
			playbackSettings = displayDialog->getPlaybackSettings();
			samplingSettings = displayDialog->getSamplingSettings();
			ui->formulaEdit->setText( displayDialog->getFormula() );
			parametersDialog->updateView();
			emit changed(UpdateInfo{
					.formula = getFormula(),
					.parameters = getParameters(),
					.parameterDescriptions = getParameterDescriptions(),
					.stateDescriptions = getStateDescriptions(),
					.playbackSettings = playbackSettings,
					.samplingSettings = getSamplingSettings()
			});
		}
	);
	connect(
		this->graphView,
		&GraphView::viewChanged,
		[this]() {
			emit viewParamsChanged();
		}
	);
}

FunctionView::~FunctionView()
{
    delete ui;
}

QString FunctionView::getFormula() {
	return ui->formulaEdit->text();
}

const ParameterDescriptions& FunctionView::getParameterDescriptions() const
{
	return dataDescription.parameterDescriptions;
}

const ParameterBindings& FunctionView::getParameters() const
{
	return parameters;
}

StateDescriptions FunctionView::getStateDescriptions() const
{
	return dataDescription.stateDescriptions;
}

const FunctionViewData& FunctionView::getViewData() const {
	return viewData;
}

const SamplingSettings& FunctionView::getSamplingSettings() const {
	return samplingSettings;
}

bool FunctionView::getIsPlaybackEnabled() const
{
	return ui->playbackEnabled->isChecked();
}

void FunctionView::setFormula( const QString& str ) {
	ui->formulaEdit->setText( str );
}

void FunctionView::setParameters( const ParameterBindings& value )
{
	parameters = value;
}

void FunctionView::setSamplingSettings(const SamplingSettings& value)
{
	samplingSettings = value;
}

void FunctionView::setGraph(
    const std::vector<std::pair<C,C>>& values
)
{
	statusBar->setVisible(false);
	graphView->setGraph( values );
}

void FunctionView::setFormulaError( const QString& str )
{
	statusBar->setVisible(true);
	statusBar->showMessage(str);
	graphView->reset();
}

void FunctionView::disablePlaybackPosition()
{
	graphView->disablePlaybackCursor();
}

void FunctionView::setPlaybackTime( const double value )
{
	graphView->setPlaybackCursor(
			value * (*globalPlaybackSpeed) * playbackSettings.playbackSpeed,
			(*globalPlaybackSpeed) * playbackSettings.playbackSpeed
	);
}

void FunctionView::focusFormula() {
	ui->formulaEdit-> setFocus();
}
void FunctionView::focusGraph() {
	graphView->setFocus();
}

void FunctionView::openDisplayDialog()
{
	displayDialog->setFormula( ui->formulaEdit->text() );
	displayDialog->setDataDescription(
			functionDataDescriptionToString( dataDescription )
	);
	displayDialog->setViewData( viewData );
	displayDialog->setPlaybackSettings( playbackSettings );
	displayDialog->setSamplingSettings( samplingSettings ),
	displayDialog->show();
}

void FunctionView::togglePlaybackEnabled()
{
	ui->playbackEnabled->toggle();
}

void updateParameters(
		const std::map<QString,ParameterDescription>& parameterDescriptions,
		ParameterBindings& parameters
)
{
	// delete obsolete entries:
	{
		auto it = parameters.begin();
		while( it != parameters.end() )
		{
			if( parameterDescriptions.find( it->first ) == parameterDescriptions.end() )
			{
				it = parameters.erase( it );
			}
			else{ ++it; }

		}
	}
	// add new entries:
	{
		auto it = parameterDescriptions.begin();
		while( it != parameterDescriptions.end() )
		{
			parameters.try_emplace(
					it->first,
					C(it->second.initial,0)
			);
			it++;
		}
	}
}
