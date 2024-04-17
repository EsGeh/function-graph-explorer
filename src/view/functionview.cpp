#include "view/functionview.h"
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
	ui->verticalLayout->addWidget( graphView, 1 );

	displayDialog = new FunctionDisplayOptions(
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
		ui->optionsBtn,
		&QAbstractButton::clicked,
		[this]() {
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

const FunctionViewData& FunctionView::getViewData() const {
	return viewData;
}

const SamplingSettings& FunctionView::getSamplingSettings() const {
	return samplingSettings;
}

void FunctionView::setFormulaError( const QString& str )
{
	statusBar->setVisible(true);
	statusBar->showMessage(str);
	graphView->reset();
}

void FunctionView::setSamplingSettings(const SamplingSettings& value)
{
	samplingSettings = value;
}

void FunctionView::setFormula( const QString& str ) {
	ui->formulaEdit->setText( str );
}

void FunctionView::setGraph(
    const std::vector<std::pair<C,C>>& values
)
{
	statusBar->setVisible(false);
	graphView->setGraph( values );
}
