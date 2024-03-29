#ifndef FUNCTIONVIEW_H
#define FUNCTIONVIEW_H

#include <QWidget>
#include <QStatusBar>
#include "graphview.h"
#include "functiondisplayoptions.h"

namespace Ui {
class FunctionView;
}

class FunctionView : public QWidget
{
  Q_OBJECT

public:
	explicit FunctionView(
			const QString& title,
			QWidget *parent = nullptr
	);
	~FunctionView();

	QString getFormula();
	GraphView* getGraphView();

  void setFormula( const QString& str );

  void setGraph(
      const std::vector<std::pair<C,C>>& values
  );
  void setFormulaError( const QString& str );

signals:
  void formulaChanged();
  void viewParamsChanged();

private:
	Ui::FunctionView *ui;
  GraphView* graphView;
	FunctionDisplayOptions* displayDialog;
	QStatusBar* statusBar;
};

#endif // FUNCTIONVIEW_H
