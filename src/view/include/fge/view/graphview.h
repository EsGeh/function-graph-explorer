#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "fge/view/viewdata.h"
#include <QChartView>


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

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;

signals:
	void viewChanged();

private:
	std::pair<T,T> getScale() const;
	void updateAxes();

	void wheelEvent(QWheelEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void moveView(QPoint direction);
	void zoomView(QPoint direction);
	void resetTranslation();
	void resetZoom();

private:
	FunctionViewData* viewData;

};

#endif // GRAPHVIEW_H
