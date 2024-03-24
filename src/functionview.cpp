#include "functionview.h"
#include "ui_functionview.h"

FunctionView::FunctionView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FunctionView)
{
	ui->setupUi(this);

	chartView = new GraphView();
	ui->verticalLayout->addWidget(chartView);

	connect(
		ui->formulaEdit,
		&QLineEdit::textChanged,
		[=](QString value) {
			emit formulaChanged();
		}
	);
	connect(
		this->chartView,
		&GraphView::viewChanged,
		[=]() {
			ui->originX->setValue(
					chartView->getOrigin().first
			);
			ui->originY->setValue(
					chartView->getOrigin().second
			);
			ui->scaleX->setValue(
					chartView->getScale().first
			);
			ui->scaleY->setValue(
					chartView->getScale().second
			);
			emit viewParamsChanged();
		}
	);

	connect(
		ui->originX,
		&QDoubleSpinBox::valueChanged,
		[=](double value) {
			chartView->setOrigin( {
					value,
					chartView->getOrigin().second
			} );
			emit viewParamsChanged();
		}
	);

	connect(
		ui->originY,
		&QDoubleSpinBox::valueChanged,
		[=](double value) {
			chartView->setOrigin( {
					chartView->getOrigin().first,
					value
			} );
			emit viewParamsChanged();
		}
	);

	connect(
		ui->scaleX,
		&QDoubleSpinBox::valueChanged,
		[=](double value) {
			chartView->setScale( {
					value,
					chartView->getScale().second
			} );
			emit viewParamsChanged();
		}
	);

	connect(
		ui->scaleY,
		&QDoubleSpinBox::valueChanged,
		[=](double value) {
			chartView->setScale( {
					chartView->getScale().first,
					value
			} );
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
GraphView* FunctionView::getGraphView() {
	return chartView;
}

void FunctionView::setFormulaError( const QString& str )
{
	chartView->reset();
}

void FunctionView::setFormula( const QString& str ) {
	ui->formulaEdit->setText( str );
}

void FunctionView::setGraph(
    const std::vector<std::pair<T,T>>& values
)
{
	chartView->setGraph( values );
}
