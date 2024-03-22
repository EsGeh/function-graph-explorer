#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "model.h"
#include "graphview.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(
    QWidget *parent = nullptr
  );
  ~MainWindow();

	QString getFormula();
	GraphView* getGraphView();

  void setFormula( const QString& str );

  void setGraph(
      const std::vector<std::pair<T,T>>& values
  );
  void setFormulaError( const QString& str );

signals:
  void updateGraph();

private:
    Ui::MainWindow *ui;
    GraphView* chartView;
};

#endif // MAINWINDOW_H
