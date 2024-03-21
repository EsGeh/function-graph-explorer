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

	void init();

  void set_formula_error( const QString& str );
  void set_graph(
      const std::vector<std::pair<T,T>>& values
  );

signals:
  void formulaChanged(QString formula);

private:
    Ui::MainWindow *ui;
    QChartView* chartView;

		void init_graph();
};
#endif // MAINWINDOW_H
