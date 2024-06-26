#include "fge/view/parametersedit.h"
#include "include/fge/view/parametervalueentry.h"
#include "ui_parametersedit.h"
#include <memory>

ParametersEdit::ParametersEdit(
		const QString& functionName,
		ParameterBindings* parameters,
		std::map<QString,ParameterDescription>* parameterDescriptions,
		QWidget *parent
)
    : QDialog(parent)
    , ui(new Ui::ParametersEdit)
		, parameters(parameters)
		, parameterDescriptions(parameterDescriptions)
{
	ui->setupUi(this);
	this->setWindowTitle( functionName );
}

ParametersEdit::~ParametersEdit()
{
	delete ui;
}

void ParametersEdit::updateView()
{
	for( auto widget : paramWidgets )
	{
		ui->verticalLayout->removeWidget( widget.get() );
	}
	paramWidgets.clear();
	for( auto& param : *parameters )
	{
		auto description = parameterDescriptions->at(param.first);
		auto paramWidget = std::make_shared<ParameterValueEntry>(
				param.first,
				param.second,
				description
		);
		paramWidgets.push_back( paramWidget );
		ui->verticalLayout->addWidget(
				paramWidget.get()
		);
		connect(
				paramWidget.get(),
				&ParameterValueEntry::valueChanged,
				[this,&param](auto value) {
					param.second = C(value, 0);
					emit parameterChanged(param.first, param.second );
				}
		);
	}
}
