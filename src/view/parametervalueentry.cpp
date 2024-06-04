#include "fge/view/parametervalueentry.h"
#include "ui_parametervalueentry.h"
#include <qslider.h>
#include <qspinbox.h>

const int sliderDiv = 10;

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
	ui->slider->setRange( description.min*sliderDiv, description.max*sliderDiv);
	// ui->slider->setSingleStep( description.step );
	connect(
		ui->value,
		&QDoubleSpinBox::valueChanged,
		[this]() {
			const auto value = ui->value->value();
			// update slider
			{
				auto blockedOld = ui->slider->blockSignals(true);
				ui->slider->setValue( value*sliderDiv );
				ui->slider->blockSignals(blockedOld);
			}
			emit valueChanged(C(value,0));
		}
	);
	connect(
		ui->slider,
		&QSlider::valueChanged,
		[this]() {
			const auto value = double(ui->slider->value()) / sliderDiv;
			// update spinbox:
			{
				auto blockedOld = ui->value->blockSignals(true);
				ui->value->setValue( value );
				ui->value->blockSignals(blockedOld);
			}
			emit valueChanged(C(value,0));
		}
	);

	{
		auto blockedOld = ui->value->blockSignals(true);
		ui->value->setValue( param.c_.real() );
		ui->value->blockSignals(blockedOld);
	}
	{
		auto blockedOld = ui->slider->blockSignals(true);
		ui->slider->setValue( param.c_.real()*sliderDiv );
		ui->slider->blockSignals(blockedOld);
	}
}

ParameterValueEntry::~ParameterValueEntry()
{
    delete ui;
}
