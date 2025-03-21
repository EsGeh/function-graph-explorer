#include "fge/view/functiondisplayoptions.h"
#include "ui_functiondisplayoptions.h"
#include <QMenuBar>
#include <QAction>
#include <qcheckbox.h>
#include <qnamespace.h>
#include <qspinbox.h>
#include <variant>


struct Template {
	QString name;
	QString formula;
	QString data;
};

const std::vector<Template> templates = {
	{
		.name = "Oscillation",
		.formula = (QStringList {
			"amp * cos( freq * 2pi*x )"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq 1 0 1000 0 ramp=volume",
			"parameter 1 amp 1 0 1 0 ramp=parameter",
		}).join("\n")
	},
	{
		.name = "Square Wave",
		.formula = (QStringList {
			"sgn(cos( freq * 2pi*x ))"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq 1 0 1000 0 ramp=volume",
		}).join("\n")
	},
	{
		.name = "Sawtooth Wave",
		.formula = (QStringList {
			"frac(freq * x)"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq 1 0 1000 0 ramp=volume",
		}).join("\n")
	},
	{
		.name = "Advanced Sawtooth Wave",
		.formula = (QStringList {
			"var x_ := x * freq;",
			"if( (x_%1) < shape, 1/shape*(x_%1), 1-1/(1-shape)*((x_-shape)%1) );"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq 1 0 1000 0 ramp=volume",
			"parameter 1 shape 1 0 1 0 ramp=parameter"
		}).join("\n")
	},
	{
		.name = "Amp Envelope",
		.formula = (QStringList {
			"(0.5 -0.5*cos(2pi * (x%1)^(1/shape)))"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 shape 1 0.1 8 0 ramp=parameter",
		}).join("\n")
	},
	{
		.name = "Random Stairs",
		.formula = (QStringList {
			"if( floor(x) != s ) {",
  		"  r := complex(rnd(),rnd());",
			"  s := floor(x);",
			"};",
			"r;"
		}).join("\n"),
		.data = (QStringList {
			"state 1 s",
			"state 1 r"
		}).join("\n")
	},
	{
		.name = "Harm. Series",
		.formula = (QStringList {
			"var acc := 0;",
			"for( var k:=1; k<=N; k+=1 ) {",
  		"  acc += (1/decay)^(k-1) * cos(k*freq*2pi*x);",
			"};",
			"amp * 1/N * acc;"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq 1 0 1000 0 ramp=volume",
			"parameter 1 amp 1 0 1 0.05 ramp=parameter",
			"parameter 1 N 1 1 16 1 ramp=volume",
			"parameter 1 decay 1 1 100 1 ramp=parameter",
		}).join("\n")
	},
	{
		.name = "Complex Oscillation",
		.formula = (QStringList {
			"exp( freq*i*2pi*x )"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq 1 0 1000 0 ramp=volume",
		}).join("\n")
	},
	{
		.name = "DFT (naive)",
		.formula = (QStringList {
			"var N := 16;",
			"var acc := 0;",
			"for( var k:=0; k<N; k+=1 ) {",
			"  acc += exp(-i*2pi*k*x/N) * f0(k/N);",
			"};",
			"1/N*acc;"
		}).join("\n")
	},
	{
		.name = "DFT (buffered)",
		.formula = (QStringList {
			"if( filled = 0 ) {",
			"  for( var j:=0; j<buffer[]; j+=1 ) {",
			"  for( var k:=0; k<buffer[]; k+=1 ) {",
			"    buffer[j] += exp(-i*2pi*k*j/buffer[]) * f0(k/buffer[]);",
			"  };",
			"  buffer[j] /= buffer[];",
			"  };",
			"  filled := 1;",
			"};",
			"buffer[x%buffer[]];"
		}).join("\n"),
		.data = (QStringList {
			"state 256 buffer",
			"state 1 filled"
		}).join("\n")
	},
	{
		.name = "Inverse DFT",
		.formula = (QStringList {
			"if( filled = 0 ) {",
			"  for( var j:=0; j<buffer[]; j+=1 ) {",
			"  for( var k:=0; k<buffer[]; k+=1 ) {",
			"    buffer[j] += exp(i*2pi*k*j/buffer[]) * f1(k);",
			"  };",
			"  };",
			"  filled := 1;",
			"};",
			"buffer[(x*buffer[])%buffer[]];"
		}).join("\n"),
		.data = (QStringList {
			"state 256 buffer",
			"state 1 filled"
		}).join("\n")
	},
	{
		.name = "FFT",
		.formula = (QStringList {
			"var N := buffer[];",
			"",
			"if( filled = 0 ) {",
			"",
			"  var samples[32768];",
			"  for( var k:=0; k<N; k+=1 ) {",
			"    samples[k] := f0(k/N);",
			"  }",
			"",
			"  bitrevcpy( buffer, samples );",
			"  // s = 1, ... , log2(N)",
			"  // m = 2, ... , N",
			"  for( var s:=1; s+0.5<log2(N)+1; s+=1 ) {",
			// "    print( 's = ', s );",
			"    var m := floor(2^s+.1);",
			"    var omega_step := exp( -2pi * i / m );",
			"    // k = 0, ... , N-1",
			"    for( var k:=0; k+0.5<N; k+=m ) {",
			// "      print( 'k = ', k );",
			"      var omega := 1;",
			"      for( var j:=0; j+0.5<m/2; j+=1 ) {",
			// "        print( 'j = ', j );",
			"        var lower := buffer[k+j];",
			"        var upper := omega * buffer[k+j+m/2];",
			"        buffer[k+j] := lower + upper;",
			"        buffer[k+j+m/2] := lower - upper;",
			"        omega *= omega_step;",
			"      }",
			"    }",
			"  };",
			"  filled := 1;",
			"};",
			"buffer[floor(x)%N]/N;"
		}).join("\n"),
		.data = (QStringList {
			"state 32768 buffer",
			"state 1 filled"
		}).join("\n")
	},
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
	for( auto templateDescr : templates ) {
		auto action = menu1->addAction( templateDescr.name );
		connect( action, &QAction::triggered,
				[this,templateDescr]() {
					ui->largeFormulaEdit->setPlainText(
							templateDescr.formula
					);
					ui->parametersEdit->setPlainText(
							templateDescr.data
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
			ui->centeredX, &QCheckBox::checkStateChanged,
			[this](bool value){ this->viewData.originCentered.first = value; }
	);
	connect(
			ui->centeredY, &QCheckBox::checkStateChanged,
			[this](bool value){ this->viewData.originCentered.second = value; }
	);
	// displayImaginary:
	connect(
			ui->displayImaginary, &QCheckBox::checkStateChanged,
			[this](bool value){ this->viewData.displayImaginary = value; }
	);
	// autoScrollOnPlayback:
	connect(
			ui->autoScrollOnPlayback, &QCheckBox::checkStateChanged,
			[this](bool value){ this->viewData.autoScrollOnPlayback = value; }
	);
	// playback:
	connect(
			ui->speed, &QDoubleSpinBox::valueChanged,
			[this](double value){ this->playbackSettings.playbackSpeed = value; }
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
			ui->periodic, &QDoubleSpinBox::valueChanged,
			[this](int value){ this->samplingSettings.periodic = value; }
	);
	connect(
			ui->buffered, &QCheckBox::checkStateChanged,
			[this](int value){ this->samplingSettings.buffered = value; }
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


QString FunctionDisplayOptions::getDataDescription() const
{
	return ui->parametersEdit->toPlainText();
}

const FunctionViewData& FunctionDisplayOptions::getViewData() const
{
	return viewData;
}

const PlaybackSettings& FunctionDisplayOptions::getPlaybackSettings() const
{
	return playbackSettings;
}

const SamplingSettings& FunctionDisplayOptions::getSamplingSettings() const
{
	return samplingSettings;
}

void FunctionDisplayOptions::setFormula(const QString& value) {
	ui->largeFormulaEdit->setPlainText( value );
}

void FunctionDisplayOptions::setDataDescription(const QString& value ) {
	ui->parametersEdit->setPlainText( value );
}

void FunctionDisplayOptions::setViewData(const FunctionViewData& value) {
	this->viewData = value;
	updateView();
}

void FunctionDisplayOptions::setPlaybackSettings(const PlaybackSettings& value)
{
	playbackSettings = value;
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
	// autoScrollOnPlayback:
	ui->autoScrollOnPlayback->setChecked( viewData.autoScrollOnPlayback );

	ui->speed->setValue( playbackSettings.playbackSpeed );

	// samplingSettings:
	ui->resolution->setValue( samplingSettings.resolution );
	ui->interpolation->setValue( samplingSettings.interpolation );
	ui->periodic->setValue( samplingSettings.periodic );
	ui->buffered->setChecked( samplingSettings.buffered );
}
