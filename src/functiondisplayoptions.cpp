#include "functiondisplayoptions.h"
#include "ui_functiondisplayoptions.h"
#include <QMenuBar>
#include <QAction>

FunctionDisplayOptions::FunctionDisplayOptions(QWidget *parent)
	: QDialog(parent)
  , ui(new Ui::FunctionDisplayOptions)
{
	ui->setupUi(this);
	auto menuBar = new QMenuBar(this);
	ui->verticalLayout->insertWidget( 0, menuBar );
	QMenu* menu1 = menuBar->addMenu("Templates");
	{
		auto action = menu1->addAction("Oscillation");
		connect( action, &QAction::triggered,
				[this]() {
					ui->largeFormulaEdit->setPlainText(
							(QStringList {
								"var freq := 440;",
								"cos( freq * 2pi*x )"
							}).join("\n")
					);
				}
		);
	}
	{
		auto action = menu1->addAction("Series");
		connect( action, &QAction::triggered,
				[this]() {
					ui->largeFormulaEdit->setPlainText(
							(QStringList {
								"var acc := 0;",
								"var max_freq := 4;",
								"for( var k:=1; k<=max_freq; k+=1 ) {",
								"  acc += (f0(k*x) );",
								"};",
								"1/max_freq*acc;"
							}).join("\n")
					);
				}
		);
	}
	{
		auto action = menu1->addAction("Complex Oscillation");
		connect( action, &QAction::triggered,
				[this]() {
					ui->largeFormulaEdit->setPlainText(
							(QStringList {
								"var freq := 440;",
								"exp( req * i*2pi*x )"
							}).join("\n")
					);
				}
		);
	}
	{
		auto action = menu1->addAction("Fourier Transform");
		connect( action, &QAction::triggered,
				[this]() {
					ui->largeFormulaEdit->setPlainText(
							(QStringList {
								"var N := 16;",
								"var acc := 0;",
								"for( var k:=1; k<=N; k+=1 ) {",
								"  acc += ( e^(-i*2pi*k*x/N)* f0(k/N) );",
								"};",
								"1/N*acc;"
							}).join("\n")
					);
				}
		);
	}
}

FunctionDisplayOptions::~FunctionDisplayOptions()
{
	delete ui;
}

QString FunctionDisplayOptions::getFormula() const {
	return ui->largeFormulaEdit->toPlainText();
}

std::pair<T,T> FunctionDisplayOptions::getOrigin() const {
	return {
		ui->originX->value(),
		ui->originY->value()
	};
}

std::pair<T,T> FunctionDisplayOptions::getScale() const {
	return {
		ui->scaleX->value(),
		ui->scaleY->value()
	};
}

T FunctionDisplayOptions::getPlaybackDuration() const {
	return ui->duration->value();
}

T FunctionDisplayOptions::getPlaybackSpeed() const {
	return ui->speed->value();
}

T FunctionDisplayOptions::getPlaybackOffset() const {
	return ui->offset->value();
}

void FunctionDisplayOptions::setFormula( const QString& value ) {
	ui->largeFormulaEdit->setPlainText( value );
}

void FunctionDisplayOptions::setOrigin( const std::pair<T,T>& value ) {
	ui->originX->setValue( value.first );
	ui->originY->setValue( value.second );
}

void FunctionDisplayOptions::setScale( const std::pair<T,T>& value ) {
	ui->scaleX->setValue( value.first );
	ui->scaleY->setValue( value.second );
}

void FunctionDisplayOptions::setPlaybackDuration( const T value ) {
	ui->duration->setValue( value );
}

void FunctionDisplayOptions::setPlaybackSpeed( const T value ) {
	ui->speed->setValue( value );
}

void FunctionDisplayOptions::setPlaybackOffset( const T value ) {
	ui->offset->setValue( value );
}
