#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QChart>
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
		[=](QString val) {
			// qDebug() << "changed";
			emit formulaChanged(val);
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

void MainWindow::init()
{
	ui->formulaEdit->setText("x^2");
}

void MainWindow::set_formula_error( const QString& str )
{
  // qWarning() << str;
	init_graph();
}

void MainWindow::set_graph(
    const std::vector<std::pair<T,T>>& values
)
{
  // qDebug() << "setting graph";
	init_graph();
	QChart *chart = chartView->chart();

	QLineSeries* series0 = new QLineSeries();
	{
		for( auto val: values ) {
			*series0 << QPointF( val.first, val.second );
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
