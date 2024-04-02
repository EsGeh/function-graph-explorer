#include "view/graphview.h"
#include <math.h>
#include <QValueAxis>
#include <QtCharts/QLineSeries>


GraphView::GraphView(
		FunctionViewData* viewData,
		QWidget *parent
)
  : QChartView{parent}
	, viewData( viewData )
{
	setAlignment(Qt::AlignRight);
	setBackgroundBrush(QBrush(Qt::white));
	setAutoFillBackground(true);
	setRenderHint(QPainter::Antialiasing);

	QChart *chart = new QChart();
	chart->legend()->hide();
  setChart( chart );
	// X:
	{
		QValueAxis* axis = new QValueAxis();
		chart->addAxis( axis, Qt::AlignBottom );
	}
	// Y:
	{
		QValueAxis* axis = new QValueAxis();
		chart->addAxis( axis, Qt::AlignLeft );
	}
	updateAxes();
}

void GraphView::setGraph(
	const std::vector<std::pair<C,C>>& values
)
{
	reset();
	QChart *chart = this->chart();

	if(viewData->displayImaginary) {

		auto series = new QLineSeries();
		for( auto value: values ) {
			*series << QPointF( value.first.c_.real(), value.second.c_.imag() );
		}
		QPen pen("#33cc33");
		pen.setWidth(2);
		series->setPen(pen);
		chart->addSeries(series);
		series->attachAxis( chart->axes(Qt::Horizontal).first() );
		series->attachAxis( chart->axes(Qt::Vertical).first() );
	}
	{

		auto series = new QLineSeries();
		for( auto value: values ) {
			*series << QPointF( value.first.c_.real(), value.second.c_.real() );
		}
		QPen pen("#3399ff");
		pen.setWidth(2);
		series->setPen(pen);
		chart->addSeries(series);
		series->attachAxis( chart->axes(Qt::Horizontal).first() );
		series->attachAxis( chart->axes(Qt::Vertical).first() );
	}
	updateAxes();
}

QSize GraphView::minimumSizeHint() const {
	return {200, 200};
}

QSize GraphView::sizeHint() const {
	return {200, 200};
}

void GraphView::updateAxes() {
	QChart *chart = this->chart();
	chart->axes(Qt::Horizontal).first()->setRange(
			viewData->getXRange().first,
			viewData->getXRange().second
	);
	chart->axes(Qt::Vertical).first()->setRange(
			viewData->getYRange().first,
			viewData->getYRange().second
	);
}

void GraphView::reset() {
	QChart *chart = this->chart();
	chart->removeAllSeries();
}

void GraphView::wheelEvent(QWheelEvent *event) {


	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta() / 8;

	auto val = 0;
	if (!numPixels.isNull()) {
		val = numPixels.y();
	} else if (!numDegrees.isNull()) {
		QPoint numSteps = numDegrees / 15;
		val = numSteps.y();
	}
	const double stepMultiplier = 0.5;
	double step = 0;
	if( val > 0 ) {
		step = -1;
	}
	else if( val < 0 ) {
		step = +1;
	}
	step *= stepMultiplier;
	switch ( event->modifiers() ) {
		case Qt::NoModifier:
			viewData->scaleExp.first += step;
			viewData->scaleExp.second += step;
		break;
		case Qt::ShiftModifier:
			viewData->scaleExp.first += step;
		break;
		case Qt::ControlModifier:
			viewData->scaleExp.second += step;
		break;
	}
	emit viewChanged();
}
