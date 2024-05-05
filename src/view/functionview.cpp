#include "fge/view/functionview.h"
#include "include/fge/view/functiondisplayoptions.h"
#include "include/fge/view/parameter_utils.h"
#include "include/fge/view/parametersedit.h"
#include "ui_functionview.h"
#include <qcheckbox.h>

FunctionView::FunctionView(
		const QString& title,
		QWidget *parent
)
    : QWidget(parent)
    , ui(new Ui::FunctionView)
		, graphView(nullptr)
		, displayDialog(nullptr)
		, statusBar(nullptr)
		, viewData()
		, samplingSettings()
{
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

	parametersDialog = new ParametersEdit(
			&parameters,
			&dataDescription.parameterDescriptions,
			this
	);
	displayDialog = new FunctionDisplayOptions(
			descrFromParameters( parameters ),
			viewData,
			samplingSettings,
			this
	);

	connect(
		ui->formulaEdit,
		&QLineEdit::textChanged,
		[this](QString value) {
			displayDialog->setFormula( value );
			emit formulaChanged();
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
		&ParametersEdit::parametersChanged,
		[this]() {
			emit formulaChanged();
		}
	);
	connect(
		ui->optionsBtn,
		&QAbstractButton::clicked,
		[this]() {
			displayDialog->setDataDescription(
					functionDataDescriptionToString( dataDescription )
			);
			displayDialog->setViewData( viewData );
			displayDialog->setSamplingSettings( samplingSettings ),
			displayDialog->show();
		}
	);
	connect(
		ui->playbackEnabled,
		&QCheckBox::stateChanged,
		[this](auto value) {
			emit playbackEnabledChanged(value != 0);
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
			samplingSettings = displayDialog->getSamplingSettings();
			ui->formulaEdit->setText( displayDialog->getFormula() );
			parametersDialog->updateView();
			emit formulaChanged();
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
					std::vector<C>(1,C(it->second.initial,0))
			);
			it++;
		}
	}
}
