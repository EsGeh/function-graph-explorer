#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QChart>
#include <QDoubleSpinBox>
#include <QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <algorithm>


MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent)
		, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

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

	chartView = new QChartView();
	ui->verticalLayout->addWidget(chartView);
	chartView->setAlignment(Qt::AlignRight);
	chartView->setBackgroundBrush(QBrush(Qt::white));
	chartView->setAutoFillBackground(true);

	QChart *chart = new QChart();
  chartView->setChart( chart );
	chart->setTitle("Function Graph");
	init_graph();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::setFormulaError( const QString& str )
{
  // qWarning() << str;
	init_graph();
}

void MainWindow::setFormula( const QString& str ) {
	ui->formulaEdit->setText( str );
}

void MainWindow::setGraph(
    const std::vector<std::pair<T,T>>& values
)
{
  // qDebug() << "setting graph";
	init_graph();
	QChart *chart = chartView->chart();

	QLineSeries* series0 = new QLineSeries();
	{
		for( auto value: values ) {
			*series0 << QPointF( value.first, value.second );
		}
		series0->setName("f(x)");
	}

	chart->addSeries(series0);
	// create Axes based on series:
	// chart->createDefaultAxes();

  // Axes:
  {
    T min = (float )std::min_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.first < v2.first; })->first;
    T max =  (float )std::max_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.first < v2.first; })->first;
    chart->axes(Qt::Horizontal).first()->setRange(
        min,
        max
    );
  }
  {
    T min = (float )std::min_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.second < v2.second; })->second;
    T max = (float )std::max_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.second < v2.second; })->second;
    chart->axes(Qt::Vertical).first()->setRange(
        min,
        max
    );
  }

	chartView->fitInView(chart);
}

void MainWindow::setXRange( T xMin, T xMax ) {
	ui->xMin->setValue( xMin );
	ui->xMax->setValue( xMax );
}

void MainWindow::init_graph() {
	QChart *chart = chartView->chart();
	chart->removeAllSeries();
	for( auto axis: chart->axes() ) {
		chart->removeAxis( axis );
	}
	// X:
	{
		QValueAxis* axis = new QValueAxis();
		axis->setMin(-1); axis->setMax(1);
		chart->addAxis( axis, Qt::AlignBottom );
	}
	// Y:
	{
		QValueAxis* axis = new QValueAxis();
		axis->setMin(-1); axis->setMax(1);
		chart->addAxis( axis, Qt::AlignLeft );
	}
}
