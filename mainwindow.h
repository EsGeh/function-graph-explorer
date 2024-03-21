#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "model.h"

#include <QMainWindow>
#include <QChartView>

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

  void setFormulaError( const QString& str );
  void setFormula( const QString& str );
  void setGraph(
      const std::vector<std::pair<T,T>>& values
  );
  void setXRange( T xMin, T xMax );

signals:
  void formulaChanged(QString formula);
  void xMinChanged(T value);
  void xMaxChanged(T value);
  void xMinReset();
  void xMaxReset();

private:
    Ui::MainWindow *ui;
    QChartView* chartView;

		void init_graph();
};

#endif // MAINWINDOW_H
