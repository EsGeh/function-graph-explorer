#include "graphview.h"
#include <math.h>
#include <QValueAxis>
#include <QtCharts/QLineSeries>

GraphView::GraphView(QWidget *parent)
    : QChartView{parent}
{
	setAlignment(Qt::AlignRight);
	setBackgroundBrush(QBrush(Qt::white));
	setAutoFillBackground(true);

	QChart *chart = new QChart();
  setChart( chart );
	chart->setTitle("Function Graph");
	// X:
	{
		QValueAxis* axis = new QValueAxis();
		axis->setMin(1); axis->setMax(1);
		chart->addAxis( axis, Qt::AlignBottom );
	}
	// Y:
	{
		QValueAxis* axis = new QValueAxis();
		axis->setMin(-1); axis->setMax(1);
		chart->addAxis( axis, Qt::AlignLeft );
	}
}

std::pair<T,T> GraphView::getXRange() const {
	QChart *chart = this->chart();
	return {
		dynamic_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first())->min(),
		dynamic_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first())->max()
	};
}
std::pair<T,T> GraphView::getYRange() const {
	QChart *chart = this->chart();
	return {
		dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical).first())->min(),
		dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical).first())->max()
	};
}

void GraphView::wheelEvent(QWheelEvent *event) {
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;

    if (!numPixels.isNull()) {
        emit zoom(numPixels.y());
    } else if (!numDegrees.isNull()) {
        QPoint numSteps = numDegrees / 15;
        emit zoom(numSteps.y());
    }
}

void GraphView::setGraph(
	const std::vector<std::pair<T,T>>& values
)
{
	QChart *chart = this->chart();

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

  // xrange:
  {
    T min = (float )std::min_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.first < v2.first; })->first;
    T max =  (float )std::max_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.first < v2.first; })->first;
		setXRange( { min, max } ); 
  }
  // yrange:
  {
    T min = (float )std::min_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.second < v2.second; })->second;
    T max =  (float )std::max_element(values.cbegin(), values.cend(), [](const std::pair<T,T>& v1, const std::pair<T,T>& v2){ return v1.second < v2.second; })->second;
		setYRange( { min, max } ); 
  }

	fitInView(chart);
}

void GraphView::setXRange( const std::pair<T,T>& range ) {
	QChart *chart = this->chart();
	chart->axes(Qt::Horizontal).first()->setRange(
			range.first,
			range.second
	);
}

void GraphView::setYRange( const std::pair<T,T>& range ) {
	QChart *chart = this->chart();
	chart->axes(Qt::Vertical).first()->setRange(
			range.first,
			range.second
	);
}

void GraphView::reset() {
	QChart *chart = this->chart();
	chart->removeAllSeries();
	setXRange( { -1, 1 } );
}
