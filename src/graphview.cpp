#include "graphview.h"
#include <math.h>
#include <QValueAxis>
#include <QtCharts/QSplineSeries>


GraphView::GraphView(QWidget *parent)
    : QChartView{parent},
		origin({0,0}),
		scaleExp({0,0})
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

std::pair<T,T> GraphView::getScale() const {
	return {
		pow(2,scaleExp.first),
		pow(2,scaleExp.second)
	};
}

std::pair<T,T> GraphView::getXRange() const {
	return { origin.first - getScale().first, origin.first + getScale().first };
}

std::pair<T,T> GraphView::getYRange() const {
	return { origin.second - getScale().second, origin.second + getScale().second };
}

void GraphView::mousePressEvent( QMouseEvent *event ) {
	if ( event->button() == Qt::LeftButton )
	{
		auto const widgetPos = event->position();
		auto const scenePos = mapToScene(QPoint(static_cast<int>(widgetPos.x()), static_cast<int>(widgetPos.y()))); 
		auto const chartItemPos = chart()->mapFromScene(scenePos); 
		auto const pos = chart()->mapToValue(chartItemPos); 
		setOrigin({ pos.x(), pos.y() });
	}
	emit viewChanged();
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
	auto step = 0;
	if( val > 0 ) {
		step = -1;
	}
	else if( val < 0 ) {
		step = +1;
	}
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
	// updateAxes();
	emit viewChanged();
}

void GraphView::setOrigin( const std::pair<T,T>& value ) {
	origin = value;
}

void GraphView::setScale( const std::pair<T,T>& value ) {
	scaleExp = {
		log2( value.first ),
		log2( value.second )
	};
}

void GraphView::setGraph(
	const std::vector<std::pair<T,T>>& values
)
{
	reset();
	QChart *chart = this->chart();

	auto series0 = new QSplineSeries();
	{
		for( auto value: values ) {
			*series0 << QPointF( value.first, value.second );
		}
		series0->setName("f(x)");
	}

	chart->addSeries(series0);
	series0->attachAxis( chart->axes(Qt::Horizontal).first() );
	series0->attachAxis( chart->axes(Qt::Vertical).first() );
	updateAxes();

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