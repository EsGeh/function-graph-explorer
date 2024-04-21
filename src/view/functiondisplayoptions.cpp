#include "fge/view/functiondisplayoptions.h"
#include "ui_functiondisplayoptions.h"
#include <QMenuBar>
#include <QAction>
#include <qcheckbox.h>


const std::vector<std::pair<QString,QString>> templates = {
	{ "Oscillation",
		(QStringList {
			"var freq := 440;",
			"cos( freq * 2pi*x )"
		}).join("\n")
	},
	{
		"Square Wave",
							(QStringList {
								"var freq := 440;",
								"sgn(cos( freq * 2pi*x ))"
							}).join("\n")
	},
	{
		"Sawtooth Wave",
							(QStringList {
								"var freq := 440;",
								"2*frac(freq * x)-1"
							}).join("\n")
	},
	{
		"Series",
							(QStringList {
								"var freq := 440;",
								"var N := 4;",
								"var acc := 0;",
								"for( var k:=1; k<=N; k+=1 ) {",
								"  acc += cos(k*freq*2pi*x);",
								"};",
								"1/N*acc;"
							}).join("\n")
	},
	{
		"Complex Oscillation",
							(QStringList {
								"var freq := 440;",
								"exp( freq*i*2pi*x )"
							}).join("\n")
	},
	{
		"Discrete Fourier Transform",
							(QStringList {
								"var N := 16;",
								"var acc := 0;",
								"for( var k:=0; k<N; k+=1 ) {",
								"  acc += exp(-i*2pi*k*x/N) * f0(k/N);",
								"};",
								"1/N*acc;"
							}).join("\n")
	},
	{
		"Fourier Series",
		(QStringList {
			"var N := 16;",
			"var acc := 0;",
			"for( var k:=0; k<N; k+=1 ) {",
			"  acc += exp(i*2pi*k*x/N) * f1(k);",
			"};",
			"acc;"
		}).join("\n")
	}
};

FunctionDisplayOptions::FunctionDisplayOptions(
		const FunctionViewData& viewData,
		const SamplingSettings& samplingSettings,
		QWidget *parent
)
	: QDialog(parent)
  , ui(new Ui::FunctionDisplayOptions)
	, viewData(viewData)
	, samplingSettings(samplingSettings)
{
	ui->setupUi(this);
	updateView();
	auto menuBar = new QMenuBar(this);
	ui->verticalLayout->insertWidget( 0, menuBar );
	QMenu* menu1 = menuBar->addMenu("Templates");
	for( auto templateData : templates ) {
		auto action = menu1->addAction( templateData.first );
		connect( action, &QAction::triggered,
				[this,templateData]() {
					ui->largeFormulaEdit->setPlainText(
							templateData.second
					);
				}
		);
	}
	// origin:
	connect(
			ui->originX, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.origin.first = value; }
	);
	connect(
			ui->originY, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.origin.second = value; }
	);
	// scale:
	connect(
			ui->scaleX, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.scaleExp.first = value; }
	);
	connect(
			ui->scaleY, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.scaleExp.second = value; }
	);
	// originCentered:
	connect(
			ui->centeredX, &QCheckBox::stateChanged,
			[this](bool value){ this->viewData.originCentered.first = value; }
	);
	connect(
			ui->centeredY, &QCheckBox::stateChanged,
			[this](bool value){ this->viewData.originCentered.second = value; }
	);
	// displayImaginary:
	connect(
			ui->displayImaginary, &QCheckBox::stateChanged,
			[this](bool value){ this->viewData.displayImaginary = value; }
	);
	// playback:
	connect(
			ui->duration, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.playbackDuration = value; }
	);
	connect(
			ui->speed, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.playbackSpeed = value; }
	);
	connect(
			ui->offset, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->viewData.playbackOffset = value; }
	);
	// samplingSettings:
	connect(
			ui->resolution, &QSpinBox::valueChanged,
			[this](int value){ this->samplingSettings.resolution = value; }
	);
	connect(
			ui->interpolation, &QSpinBox::valueChanged,
			[this](int value){ this->samplingSettings.interpolation = value; }
	);
	connect(
			ui->caching, &QCheckBox::stateChanged,
			[this](int value){ this->samplingSettings.caching = value; }
	);
}

FunctionDisplayOptions::~FunctionDisplayOptions()
{
	delete ui;
}

QString FunctionDisplayOptions::getFormula() const
{
	return ui->largeFormulaEdit->toPlainText();
}

const FunctionViewData& FunctionDisplayOptions::getViewData() const
{
	return viewData;
}

const SamplingSettings& FunctionDisplayOptions::getSamplingSettings() const
{
	return samplingSettings;
}

void FunctionDisplayOptions::setFormula(const QString& value) {
	ui->largeFormulaEdit->setPlainText( value );
}

void FunctionDisplayOptions::setViewData(const FunctionViewData& value) {
	this->viewData = value;
	updateView();
}

void FunctionDisplayOptions::setSamplingSettings(const SamplingSettings& value)
{
	samplingSettings = value;
	updateView();
}

void FunctionDisplayOptions::updateView() {
	// formula:
	// ui->largeFormulaEdit->setPlainText( viewData.formula );
	// origin:
	ui->originX->setValue( viewData.origin.first );
	ui->originY->setValue( viewData.origin.second );
	// scale:
	ui->scaleX->setValue( viewData.scaleExp.first );
	ui->scaleY->setValue( viewData.scaleExp.second );
	// originCentered:
	ui->centeredX->setChecked( viewData.originCentered.first );
	ui->centeredY->setChecked( viewData.originCentered.second );
	// displayImaginary:
	ui->displayImaginary->setChecked( viewData.displayImaginary );

	ui->duration->setValue( viewData.playbackDuration );
	ui->speed->setValue( viewData.playbackSpeed );
	ui->offset->setValue( viewData.playbackOffset );

	// samplingSettings:
	ui->resolution->setValue( samplingSettings.resolution );
	ui->interpolation->setValue( samplingSettings.interpolation );
	ui->caching->setChecked( samplingSettings.caching );
}
