#include "fge/view/graphview.h"
#include <QValueAxis>
#include <QtCharts/QLineSeries>
#include <qevent.h>
#include <qgraphicsitem.h>
#include <qnamespace.h>


GraphView::GraphView(
		FunctionViewData* viewData,
		QWidget *parent
)
  : QChartView{parent}
	, viewData( viewData )
	, series( nullptr )
	, playbackTimeMarker( nullptr )
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

	// imaginary:
	if(viewData->displayImaginary) {
		series = new QLineSeries();
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
	// real:
	{
		series = new QLineSeries();
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
	updateTimeMarker();
}

void GraphView::reset() {
	QChart *chart = this->chart();
	chart->removeAllSeries();
	setPlaybackTimeEnabled( false );
}

void GraphView::setPlaybackTime( const double value )
{
	timeMarker = value;
	updateTimeMarker();
}

void GraphView::setPlaybackTimeEnabled( const bool value )
{
	timeMarkerEnabled = value;
	updateTimeMarker();
}


QSize GraphView::minimumSizeHint() const {
	return {200, 200};
}

QSize GraphView::sizeHint() const {
	return {200, 200};
}

void GraphView::resizeEvent(QResizeEvent* event)
{
	QChartView::resizeEvent(event);
	updateTimeMarker();
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
	// scale:
	if( event->modifiers() & Qt::ControlModifier ) {
		if(
				(event->modifiers() & Qt::ShiftModifier) == 0
				&& (event->modifiers() & Qt::AltModifier) == 0
		) {
			if( step.x() == 0 ) step.setX( step.y() );
			else if( step.y() == 0 ) step.setY( step.x() );
		}
		step *= -1;
		zoomView( step );
		emit viewChanged();
	}
	// translation:
	else {
		moveView( step );
		emit viewChanged();
	}
}

void GraphView::keyPressEvent(QKeyEvent *event)
{
	if( event->modifiers() & Qt::ControlModifier ) {
		if( event->key() == Qt::Key_0 )
		{
			resetTranslation();
			resetZoom();
			emit viewChanged();
			return;
		}
	}
	QPoint direction = {0,0};
	// scale:
	if( event->modifiers() & Qt::ControlModifier ) {
		if( event->modifiers() & Qt::ShiftModifier ) {
			if( event->key() == Qt::Key_Down )
			{
				direction = {0,1};
			}
			else if( event->key() == Qt::Key_Up )
			{
				direction = {0,-1};
			}
		}
		else if( event->modifiers() & Qt::AltModifier ) {
			if( event->key() == Qt::Key_Down )
			{
				direction = {1,0};
			}
			else if( event->key() == Qt::Key_Up )
			{
				direction = {-1,0};
			}
		}
		else
		{
			if( event->key() == Qt::Key_Plus )
			{
				direction = {-1,-1};
			}
			else if( event->key() == Qt::Key_Minus )
			{
				direction = {1,1};
			}
			else if( event->key() == Qt::Key_Down )
			{
				direction = {0,-1};
			}
			else if( event->key() == Qt::Key_Up )
			{
				direction = {0,1};
			}
			else if( event->key() == Qt::Key_Left )
			{
				direction = {-1,0};
			}
			else if( event->key() == Qt::Key_Right )
			{
				direction = {1,0};
			}
		}
		if( direction != QPoint{0,0} )
		{
			zoomView( direction );
			emit viewChanged();
		}
	}
	// translation:
	else {
		if( event->key() == Qt::Key_Left )
			direction.setX( -1 );
		else if( event->key() == Qt::Key_Right )
			direction.setX( 1 );
		else if( event->key() == Qt::Key_Down )
			direction.setY( -1 );
		else if( event->key() == Qt::Key_Up )
			direction.setY( 1 );
		if( direction != QPoint{0,0} )
		{
			moveView( direction );
			emit viewChanged();
		}
	}
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

void GraphView::updateTimeMarker()
{
	if( !playbackTimeMarker ) {
		QPen pen("#555555");
		pen.setWidth(2);
		playbackTimeMarker = new QGraphicsLineItem();
		scene()->addItem( playbackTimeMarker );
		playbackTimeMarker->setPen( pen );
	}
	playbackTimeMarker->setLine(
			QLineF(
				chart()->mapToPosition(
					QPointF(
						timeMarker,
						viewData->getYRange().first
					)
				),
				chart()->mapToPosition(
					QPointF(
						timeMarker,
						viewData->getYRange().second
					)
				)
			)
	);
	playbackTimeMarker->setVisible( timeMarkerEnabled );
	playbackTimeMarker->setZValue(100);
}

void GraphView::moveView(QPoint direction)
{
	viewData->origin.first += direction.x() * (viewData->getXRange().second - viewData->getXRange().first)/4;
	viewData->origin.second += direction.y() * (viewData->getYRange().second - viewData->getYRange().first)/4;
}

void GraphView::zoomView(QPoint direction)
{
	viewData->scaleExp.first += direction.x();
	viewData->scaleExp.second += direction.y();
}

void GraphView::resetTranslation()
{
	viewData->origin = {0,0};
}

void GraphView::resetZoom()
{
	viewData->scaleExp = {0,0};
}
