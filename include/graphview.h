#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QChartView>

typedef float T;

class GraphView : public QChartView
{
	Q_OBJECT

public:
  explicit GraphView(QWidget *parent = nullptr);

	std::pair<T,T> getOrigin() const;
	std::pair<T,T> getScale() const;

	std::pair<T,T> getXRange() const;
	std::pair<T,T> getYRange() const;

	void setOrigin( const std::pair<T,T>& value );
	void setScale( const std::pair<T,T>& value );

	void mousePressEvent( QMouseEvent *event );
	void setGraph(
    const std::vector<std::pair<T,T>>& values
	);

	void reset();

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;

signals:
	void viewChanged();

private:
	void updateAxes();

	void wheelEvent(QWheelEvent *event);

private:
	std::pair<T,T> origin;
	std::pair<T,T> scaleExp;

	QPoint lastClickPos;

};

#endif // GRAPHVIEW_H
