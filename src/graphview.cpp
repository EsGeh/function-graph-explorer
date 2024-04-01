#include "graphview.h"
#include <math.h>
#include <QValueAxis>
#include <QtCharts/QLineSeries>


GraphView::GraphView(QWidget *parent)
  : QChartView{parent}
	, origin({0,0})
	, scaleExp({0,0})
	, originCentered({false,true})
	, displayImaginary( false )
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

std::pair<T,T> GraphView::getOrigin() const {
	return origin;
}

std::pair<T,T> GraphView::getScaleExp() const {
	return scaleExp;
}

std::pair<T,T> GraphView::getScale() const {
	return {
		pow( 2, scaleExp.first ),
		pow( 2, scaleExp.second )
	};
}

std::pair<T,T> GraphView::getXRange() const {
	return {
		originCentered.first
			? origin.first - getScale().first
			: origin.first,
		origin.first + getScale().first
	};
}

std::pair<T,T> GraphView::getYRange() const {
	return {
		originCentered.second
			? origin.second - getScale().second
			: origin.second,
		origin.second + getScale().second
	};
}

std::pair<bool,bool> GraphView::getOriginCentered() const {
	return originCentered;
}

bool GraphView::getDisplayImaginary() const {
	return displayImaginary;
}

void GraphView::setOrigin( const std::pair<T,T>& value ) {
	origin = value;
}

void GraphView::setScaleExp( const std::pair<T,T>& value ) {
	scaleExp = value;
}

void GraphView::setOriginCentered( const std::pair<bool,bool>& value ) {
	originCentered = value;
}

void GraphView::setDisplayImaginary( const bool value ) {
	displayImaginary = value;
}

void GraphView::setGraph(
	const std::vector<std::pair<C,C>>& values
)
{
	reset();
	QChart *chart = this->chart();

	if(displayImaginary) {

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
			getXRange().first,
			getXRange().second
	);
	chart->axes(Qt::Vertical).first()->setRange(
			getYRange().first,
			getYRange().second
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
			scaleExp.first += step;
			scaleExp.second += step;
		break;
		case Qt::ShiftModifier:
			scaleExp.first += step;
		break;
		case Qt::ControlModifier:
			scaleExp.second += step;
		break;
	}
	emit viewChanged();
}
