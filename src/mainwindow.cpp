#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QChart>
#include <QDoubleSpinBox>
#include <algorithm>


MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent)
		, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	chartView = new GraphView();
	ui->verticalLayout->addWidget(chartView);

	connect(
		ui->formulaEdit,
		&QLineEdit::textChanged,
		[=](QString value) {
			// qDebug() << "changed";
			emit formulaChanged();
		}
	);
	connect(
		this->chartView,
		&GraphView::viewChanged,
		[=]() {
			// qDebug() << "changed";
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

MainWindow::~MainWindow()
{
	delete ui;
}

QString MainWindow::getFormula() {
	return ui->formulaEdit->text();
}
GraphView* MainWindow::getGraphView() {
	return chartView;
}

void MainWindow::setFormulaError( const QString& str )
{
  // qWarning() << str;
	chartView->reset();
}

void MainWindow::setFormula( const QString& str ) {
	ui->formulaEdit->setText( str );
}

void MainWindow::setGraph(
    const std::vector<std::pair<T,T>>& values
)
{
  // qDebug() << "setting graph";
	chartView->setGraph( values );
}
