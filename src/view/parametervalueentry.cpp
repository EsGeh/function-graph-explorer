#include "fge/view/parametervalueentry.h"
#include "ui_parametervalueentry.h"
#include <qslider.h>
#include <qspinbox.h>

ParameterValueEntry::ParameterValueEntry(
		const QString& name,
		const C& param,
		QWidget *parent
)
    : QWidget(parent)
    , ui(new Ui::ParameterValueEntry)
{
	ui->setupUi(this);
	ui->nameLbl->setText( name );
	connect(
		ui->value,
		&QDoubleSpinBox::valueChanged,
		[this]() {
			const auto value = ui->value->value();
			emit valueChanged(C(value,0));
		}
	);

	ui->value->setValue( param.c_.real() );
}

ParameterValueEntry::~ParameterValueEntry()
{
    delete ui;
}
