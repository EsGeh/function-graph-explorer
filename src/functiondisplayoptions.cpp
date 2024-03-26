#include "functiondisplayoptions.h"
#include "ui_functiondisplayoptions.h"

FunctionDisplayOptions::FunctionDisplayOptions(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FunctionDisplayOptions)
{
    ui->setupUi(this);
}

FunctionDisplayOptions::~FunctionDisplayOptions()
{
    delete ui;
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

void FunctionDisplayOptions::setOrigin( const std::pair<T,T>& value ) {
	ui->originX->setValue( value.first );
	ui->originY->setValue( value.second );
}

void FunctionDisplayOptions::setScale( const std::pair<T,T>& value ) {
	ui->scaleX->setValue( value.first );
	ui->scaleY->setValue( value.second );
}
