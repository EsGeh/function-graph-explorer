#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fge/view/functionview.h"

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

	// GET:
	FunctionView* getFunctionView(const size_t index) const;
	size_t getFunctionViewCount() const;

	// SET:
	void resizeFunctionView(const size_t size);

	void resetPlayback();
	void setGlobalPlaybackSpeed( const double value );
	void setPlaybackTime( const double value );

signals:
	void functionCountChanged(const uint count);
	void globalPlaybackSpeedChanged(const double playbackSpeed);
	void isAudioEnabledChanged(const bool value);

private:
	Ui::MainWindow *ui;
	std::vector<FunctionView*> functionViews;
};

#endif // MAINWINDOW_H
