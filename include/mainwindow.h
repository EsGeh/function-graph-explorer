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
  void zoom(int delta);

private:
    Ui::MainWindow *ui;
    GraphView* chartView;
};

#endif // MAINWINDOW_H
