#include "fge/view/functiondisplayoptions.h"
#include "ui_functiondisplayoptions.h"
#include <QMenuBar>
#include <QAction>
#include <qcheckbox.h>
#include <qnamespace.h>
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
			"cos( freq * 2pi*x )"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq",
		}).join("\n")
	},
	{
		.name = "Square Wave",
		.formula = (QStringList {
			"sgn(cos( freq * 2pi*x ))"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq",
		}).join("\n")
	},
	{
		.name = "Sawtooth Wave",
		.formula = (QStringList {
			"2*frac(freq * x)-1"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq",
		}).join("\n")
	},
	{
		.name = "Series",
		.formula = (QStringList {
			"var N := 4;",
			"var acc := 0;",
			"for( var k:=1; k<=N; k+=1 ) {",
			"  acc += cos(k*freq*2pi*x);",
			"};",
			"1/N*acc;"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq",
		}).join("\n")
	},
	{
		.name = "Complex Oscillation",
		.formula = (QStringList {
			"exp( freq*i*2pi*x )"
		}).join("\n"),
		.data = (QStringList {
			"parameter 1 freq",
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
			"var N := 256;",
			"if( filled = 0 ) {",
			"  for( var j:=0; j<N; j+=1 ) {",
			"  for( var k:=0; k<N; k+=1 ) {",
			"    buffer[j] += exp(-i*2pi*k*j/N) * f0(k/N);",
			"  };",
			"  buffer[j] /= N;",
			"  };",
			"  filled := 1;",
			"};",
			"buffer[x%N];"
		}).join("\n"),
		.data = (QStringList {
			"state 256 buffer",
			"state 1 filled"
		}).join("\n")
	},
	{
		.name = "Inverse DFT",
		.formula = (QStringList {
			"var N := 256;",
			"if( filled = 0 ) {",
			"  for( var j:=0; j<N; j+=1 ) {",
			"  for( var k:=0; k<N; k+=1 ) {",
			"    buffer[j] += exp(i*2pi*k*j/N) * f1(k);",
			"  };",
			"  };",
			"  filled := 1;",
			"};",
			"buffer[(x*N)%N];"
		}).join("\n"),
		.data = (QStringList {
			"state 256 buffer",
			"state 1 filled"
		}).join("\n")
	},
};

FunctionDisplayOptions::FunctionDisplayOptions(
		const std::vector<QString>& parameters,
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

struct ParameterDescription {};
using StateDescription = VariableDescription;

std::map<QString,std::variant<ParameterDescription,StateDescription>> parseFunctionDataDescription(
		const QString& str
)
{
	decltype(parseFunctionDataDescription(str)) ret;
	auto qstringlist = str.split(
			"\n",
			Qt::SkipEmptyParts
	);
	for( auto lineRaw : qstringlist ) {
		auto line = lineRaw.trimmed();
		auto words = line.split(' ');
		if( words.size() < 1 ) {
			continue;
		}
		auto argOrState = words.at(0);
		// <param|argument> <size> <name>
		if( argOrState == "parameter" || argOrState == "param" ) {
			if( words.size() < 3 ) {
				continue;
			}
			ret.insert({ words.at(2), ParameterDescription{} });
		}
		else if( argOrState == "state" ) {
			if( words.size() < 3 ) {
				continue;
			}
			bool ok = true;
			auto size = words.at(1).toInt( &ok, 10);
			if( !ok || size < 0 ) {
				continue;
			}
			ret.insert({ words.at(2), VariableDescription{ .size = (uint )size } });
		}
		else {
			continue;
		}
	}
	return ret;
}

std::vector<QString> FunctionDisplayOptions::getParameters() const
{
	std::vector<QString> ret;
	auto dataDescr = parseFunctionDataDescription( ui->parametersEdit->toPlainText() );
	for( auto [name,eitherVal] : dataDescr ) {
		if( std::holds_alternative<ParameterDescription>( eitherVal ) ) {
			ret.push_back( name );
		}
	}
	return ret;
}

StateDescriptions FunctionDisplayOptions::getStateDescriptions() const
{
	decltype(getStateDescriptions()) ret;
	auto dataDescr = parseFunctionDataDescription( ui->parametersEdit->toPlainText() );
	for( auto [name,eitherVal] : dataDescr ) {
		if( auto value = std::get_if<StateDescription>( &eitherVal ) ) {
			ret.insert({ name, *value });
		}
	}
	return ret;
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

void FunctionDisplayOptions::setFunctionDataDescriptions(
		const std::vector<QString>& parameters,
		const StateDescriptions& state
)
{
	QString str = "";
	for( auto name : parameters ) {
		str += QString("parameter 1 %1\n")
			.arg( name )
		;
	}
	for( auto [name, value] : state ) {
		str += QString("state %2 %1\n")
			.arg( name )
			.arg( value.size )
		;
	}
	ui->parametersEdit->setPlainText( str );
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
