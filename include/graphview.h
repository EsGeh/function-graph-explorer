#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include "global.h"
#include <QChartView>


class GraphView : public QChartView
{
	Q_OBJECT

public:
  explicit GraphView(QWidget *parent = nullptr);

	std::pair<T,T> getOrigin() const;
	std::pair<T,T> getScaleExp() const;

	std::pair<T,T> getXRange() const;
	std::pair<T,T> getYRange() const;

	std::pair<bool,bool> getOriginCentered() const;
	bool getDisplayImaginary() const;

	void setOrigin( const std::pair<T,T>& value );
	void setScaleExp( const std::pair<T,T>& value );
	void setOriginCentered( const std::pair<bool,bool>& value );
	void setDisplayImaginary( const bool value );

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

private:
	std::pair<T,T> origin;
	std::pair<T,T> scaleExp;
	std::pair<bool,bool> originCentered;
	bool displayImaginary;

	QPoint lastClickPos;

};

#endif // GRAPHVIEW_H
