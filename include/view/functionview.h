#ifndef FUNCTIONVIEW_H
#define FUNCTIONVIEW_H

#include "view/viewdata.h"
#include "view/graphview.h"
#include "view/functiondisplayoptions.h"

#include <QWidget>
#include <QStatusBar>


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
	const FunctionViewData& getViewData() const;

  void setFormula( const QString& str );

  void setGraph(
      const std::vector<std::pair<C,C>>& values
  );
  void setFormulaError( const QString& str );

signals:
  void formulaChanged();
  void viewParamsChanged();
  void playButtonPressed(
			const T duration,
			const T speed,
			const T offset
	);

private:
	Ui::FunctionView *ui;
  GraphView* graphView;
	FunctionDisplayOptions* displayDialog;
	QStatusBar* statusBar;
	FunctionViewData viewData;
};

#endif // FUNCTIONVIEW_H
