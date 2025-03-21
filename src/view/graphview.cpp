#include "fge/view/graphview.h"
#include "fge/view/keybindings.h"
#include <QValueAxis>
#include <QtCharts/QLineSeries>
#include <qboxlayout.h>
#include <qchartview.h>
#include <qevent.h>
#include <qgraphicsitem.h>
#include <qnamespace.h>
#include <QLayout>
#include <QGraphicsLayout>
#include <qsizepolicy.h>
#include <qtversion.h>


GraphView::GraphView(
		FunctionViewData* viewData,
		QWidget *parent
)
  : QChartView{parent}
	, viewData( viewData )
	, series( nullptr )
	, playbackTimeMarker( nullptr )
{
	addGraphViewKeyCodes(this);
	QSizePolicy policy(
			QSizePolicy::MinimumExpanding,
			QSizePolicy::MinimumExpanding
	);
	setSizePolicy( policy );
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundBrush(
			this->palette().color( QPalette::Window )
	);
	setAutoFillBackground(true);
	setRenderHint(QPainter::Antialiasing);

	this->setLineWidth( 2 );
	this->setFrameStyle( QFrame::Box );
	this->setStyleSheet( "color: rgba(0,0,0,0);" );

	QChart* chart = new QChart();
  setChart( chart );
	chart->setDropShadowEnabled( false );
	chart->layout()->setContentsMargins( 0, 0, 0, 0 );
	chart->setMargins( QMargins(10,10,10,10) );
	chart->setBackgroundRoundness(0);

	chart->setBackgroundBrush(
			this->palette().color( QPalette::Window )
	);
	auto bgPenColor = this->palette().color( QPalette::WindowText );
	bgPenColor.setAlphaF( 0.5 );
	chart->setBackgroundPen(
			bgPenColor
	);
	chart->legend()->hide();
	// X:
	{
		QValueAxis* axis = new QValueAxis();
		axis->setLabelsColor( 
			bgPenColor
		);
		axis->setGridLineColor( bgPenColor );
		chart->addAxis( axis, Qt::AlignBottom );
	}
	// Y:
	{
		QValueAxis* axis = new QValueAxis();
		axis->setLabelsColor( 
			bgPenColor
		);
		axis->setGridLineColor( bgPenColor );
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

	// real:
	{
		series = new QLineSeries();
		{
			QPen pen;
			pen.setColor( "#00cbcb" );
			pen.setWidth( 2 );
			series->setPen( pen );
		}
		for( auto value: values ) {
			*series << QPointF( value.first.c_.real(), value.second.c_.real() );
		}
		chart->addSeries(series);
		series->attachAxis( chart->axes(Qt::Horizontal).first() );
		series->attachAxis( chart->axes(Qt::Vertical).first() );
	}
	// imaginary:
	if(viewData->displayImaginary) {
		series = new QLineSeries();
		{
			QPen pen;
			pen.setColor( "#004bab" );
			pen.setWidth( 2 );
			series->setPen( pen );
		}
		for( auto value: values ) {
			*series << QPointF( value.first.c_.real(), value.second.c_.imag() );
		}
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
	disablePlaybackCursor();
}

void GraphView::disablePlaybackCursor()
{
	playbackCursorEnabled = false;
	updateTimeMarker();
}

void GraphView::setPlaybackCursor(
		const double value,
		const double speed
)
{
	playbackCursorEnabled = true;
	playbackCursor = value;
	if( viewData->autoScrollOnPlayback ) {
		// if the playback cursor is out of view,
		// move the view window such that it is
		// in view again:
		auto xViewRange = viewData->getXRange();
		auto xViewSize = xViewRange.second - xViewRange.first;
		if( xViewSize/speed >= 1 )
		{
			if(
					(playbackCursor - xViewRange.second ) > 0.1
					|| (xViewRange.first - playbackCursor) > 0.1
			)
			{
				viewData->origin.first = floor( playbackCursor );
				emit viewChanged();
			}
		}
		// if the view is zoomed in a lot,
		// (= view window is very short)
		// or playback speed is very high
		// the cursor would
		// run out of view at every frame.
		// The following heuristics avoid
		// updating the view at every frame:
		else {
			// update if out ouf view for > 1sec:
			if(
					playbackCursor - (xViewRange.first + speed) > 0.1
					|| ((xViewRange.second - speed) - playbackCursor) > 0.1
			)
			{
				viewData->origin.first = floor( playbackCursor );
				emit viewChanged();
			}
		}
	}
	updateTimeMarker();
}


QSize GraphView::minimumSizeHint() const {
	return {200, 300};
}

QSize GraphView::sizeHint() const {
	return {200, 300};
}

void GraphView::resizeEvent(QResizeEvent* event)
{
	QChartView::resizeEvent(event);
	updateTimeMarker();
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
		playbackTimeMarker = new QGraphicsLineItem();
		scene()->addItem( playbackTimeMarker );
		QPen pen(
				this->palette().color( QPalette::Highlight )
		);
		pen.setWidth(2);
		playbackTimeMarker->setPen( pen );
	}
	playbackTimeMarker->setLine(
			QLineF(
				chart()->mapToPosition(
					QPointF(
						playbackCursor,
						viewData->getYRange().first
					)
				),
				chart()->mapToPosition(
					QPointF(
						playbackCursor,
						viewData->getYRange().second
					)
				)
			)
	);
	playbackTimeMarker->setVisible( playbackCursorEnabled );
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

void GraphView::focusInEvent(QFocusEvent* event) {
	this->setStyleSheet( "color: palette(highlight);" );
	this->chart()->setBackgroundBrush(
			this->palette().color( QPalette::AlternateBase )
	);
}

void GraphView::focusOutEvent(QFocusEvent* event) {
	this->setStyleSheet( "color: rgba(0,0,0,0);" );
	this->chart()->setBackgroundBrush(
			this->palette().color( QPalette::Window )
	);
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
