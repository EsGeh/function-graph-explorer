#include "fge/view/functionview.h"
#include "include/fge/view/functiondisplayoptions.h"
#include "include/fge/view/parametersedit.h"
#include "ui_functionview.h"

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
			displayDialog->setParameters(
					descrFromParameters( parameters )
			);
			displayDialog->setViewData( viewData );
			displayDialog->setSamplingSettings( samplingSettings ),
			displayDialog->show();
		}
	);
	connect(
		ui->playBtn,
		&QAbstractButton::clicked,
		[this]() {
			emit playButtonPressed(
					viewData.playbackDuration,
					viewData.playbackSpeed,
					viewData.playbackOffset
			);
		}
	);
	connect(
		displayDialog,
		&FunctionDisplayOptions::finished,
		[this](int result) {
			if( !result ) return;
			updateParameters(
					displayDialog->getParameters(),
					parameters
			);
			ui->parametersBtn->setVisible( parameters.size() > 0 );
			viewData = displayDialog->getViewData();
			samplingSettings = displayDialog->getSamplingSettings();
			ui->formulaEdit->setText( displayDialog->getFormula() );
			emit formulaChanged();
		}
	);
	connect(
		this->graphView,
		&GraphView::viewChanged,
		[this]() {
			// qDebug() << "zoom: " << viewData.scaleExp;
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
		const std::vector<QString>& parameterDescription,
		ParameterBindings& parameters
)
{
	// delete obsolete entries:
	{
		auto it = parameters.begin();
		while( it != parameters.end() )
		{
			if(
					std::find( parameterDescription.begin(), parameterDescription.end(), it->first ) == parameterDescription.end() ) {
				it = parameters.erase( it );
			}
			else{ ++it; }
		}
	}
	// add new entries:
	{
		auto it = parameterDescription.begin();
		while( it != parameterDescription.end() )
		{
			parameters.try_emplace( *it, 0 );
			it++;
		}
	}
}
