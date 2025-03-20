#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "fge/view/viewdata.h"
#include <QChartView>
#include <qgraphicsitem.h>
#include <qlineseries.h>


class GraphView : public QChartView
{
	Q_OBJECT

public:
  explicit GraphView(
			FunctionViewData* viewData,
			QWidget *parent = nullptr
	);

	void setGraph(
    const std::vector<std::pair<C,C>>& values
	);

	void reset();

	void disablePlaybackCursor();
	void setPlaybackCursor(
			const double value,
			const double speed // playback speed
	);

signals:
	void viewChanged();

public slots:

	void updateAxes();
	void updateTimeMarker();

	// Actions:
	void moveView(QPoint direction);
	void zoomView(QPoint direction);
	void resetView() { resetZoom(); resetTranslation(); }
	void resetTranslation();
	void resetZoom();

private:

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent *event) override;

protected:
	virtual void focusInEvent(QFocusEvent* event) override;
	virtual void focusOutEvent(QFocusEvent* event) override;

private:
	// Data:
	FunctionViewData* viewData;
	double playbackCursor;
	bool playbackCursorEnabled = false;
	// GUI:
	QLineSeries* series;
	QGraphicsLineItem* playbackTimeMarker;

};

#endif // GRAPHVIEW_H
