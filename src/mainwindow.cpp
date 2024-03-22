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
			emit formulaChanged(value);
		}
	);
	connect(
		ui->xMinFix,
		&QCheckBox::stateChanged,
		[=](int value) {
			ui->xMin->setEnabled(value);
			if( value == 0 ) {
				emit xMinReset();
			}
		}
	);
	connect(
		ui->xMaxFix,
		&QCheckBox::stateChanged,
		[=](int value) {
			ui->xMax->setEnabled(value);
			if( value == 0 ) {
				emit xMaxReset();
			}
		}
	);
	connect(
		ui->xMin,
		&QDoubleSpinBox::valueChanged,
		[=](double value) {
			emit xMinChanged( value );
		}
	);
	connect(
		ui->xMax,
		&QDoubleSpinBox::valueChanged,
		[=](double value) {
			emit xMaxChanged( value );
		}
	);

	connect(
		this->chartView,
		&GraphView::zoom,
		[=](int delta) {
			// qDebug() << "zoom: " << delta;
			emit zoom( delta );
		}
	);
}

MainWindow::~MainWindow()
{
	delete ui;
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
	chartView->reset();
	chartView->setGraph( values );
}

void MainWindow::setXRange( T xMin, T xMax ) {
	ui->xMin->setValue( xMin );
	ui->xMax->setValue( xMax );
}
