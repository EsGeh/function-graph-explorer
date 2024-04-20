#include "fge/view/graphview.h"
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

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void GraphView::wheelEvent(QWheelEvent *event) {

	QPoint step = {0,0};
	{
		QPoint numPixels = event->pixelDelta();
		QPoint numDegrees = event->angleDelta() / 8;

		QPoint val = {0,0};
		if( !numPixels.isNull() ) {
			val = numPixels;
		}
		else if( !numDegrees.isNull() ) {
			QPoint numSteps = numDegrees / 15;
			val = numSteps;
		}
		step.setX( sgn( val.x() ) );
		step.setY( sgn( val.y() ) );
	}
	// Zoom
	if( event->modifiers() & Qt::ControlModifier ) {
		if(
				(event->modifiers() & Qt::ShiftModifier) == 0
				&& (event->modifiers() & Qt::AltModifier) == 0
		) {
			if( step.x() == 0 ) step.setX( step.y() );
			else if( step.y() == 0 ) step.setY( step.x() );
		}
		step *= -1;
		viewData->scaleExp.first += step.x();
		viewData->scaleExp.second += step.y();
		emit viewChanged();
	}
	else {
		viewData->origin.first += step.x() * (viewData->getXRange().second - viewData->getXRange().first)/4;
		viewData->origin.second += step.y() * (viewData->getYRange().second - viewData->getYRange().first)/4;
		emit viewChanged();
	}
}
