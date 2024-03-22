#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QChartView>

typedef float T;

class GraphView : public QChartView
{
	Q_OBJECT
public:
  explicit GraphView(QWidget *parent = nullptr);

	std::pair<T,T> getXRange() const;
	std::pair<T,T> getYRange() const;

	// void mouseMoveEvent(QMouseEvent *event);
	void setGraph(
    const std::vector<std::pair<T,T>>& values
	);

	void setXRange( const std::pair<T,T>& value );
	void setYRange( const std::pair<T,T>& value );

	void reset();

private:
	void wheelEvent(QWheelEvent *event);

signals:
  void zoom(int delta);
};

#endif // GRAPHVIEW_H
