#include "fge/view/parametervalueentry.h"
#include "ui_parametervalueentry.h"
#include <qslider.h>
#include <qspinbox.h>

ParameterValueEntry::ParameterValueEntry(
		const QString& name,
		const C& param,
		const ParameterDescription& description,
		QWidget *parent
)
    : QWidget(parent)
    , ui(new Ui::ParameterValueEntry)
{
	ui->setupUi(this);
	ui->nameLbl->setText( name );
	ui->value->setRange( description.min, description.max );
	ui->value->setSingleStep(
			description.step>0 ? description.step : 0.1
	);
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
