#ifndef FUNCTIONVIEW_H
#define FUNCTIONVIEW_H

#include <QWidget>
#include "graphview.h"

namespace Ui {
class FunctionView;
}

class FunctionView : public QWidget
{
  Q_OBJECT

public:
	explicit FunctionView(QWidget *parent = nullptr);
	~FunctionView();

	QString getFormula();
	GraphView* getGraphView();

  void setFormula( const QString& str );

  void setGraph(
      const std::vector<std::pair<T,T>>& values
  );
  void setFormulaError( const QString& str );

signals:
  void formulaChanged();
  void viewParamsChanged();

private:
	Ui::FunctionView *ui;
  GraphView* chartView;
};

#endif // FUNCTIONVIEW_H
