#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "model.h"
#include "functionview.h"

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

	FunctionView* getFunctionView(const size_t index) const;
	size_t getFunctionViewCount() const;

	void resizeFunctionView(const size_t size);

signals:
	void functionCountChanged(const size_t count);

private:
	Ui::MainWindow *ui;
	std::vector<FunctionView*> functionViews;
};

#endif // MAINWINDOW_H
