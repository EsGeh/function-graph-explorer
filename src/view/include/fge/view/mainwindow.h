#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "fge/view/data.h"
#include "fge/view/functionview.h"
#include "fge/view/tipsdialog.h"
#include "fge/view/helpdialog.h"
#include "fge/view/statistics.h"
#include "statistics.h"

#include <QMainWindow>
#include <qlabel.h>

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
			const Resources* resources,
    	QWidget *parent = nullptr
  );
  ~MainWindow();

	// GET:
	FunctionView* getFunctionView(const size_t index) const;
	size_t getFunctionViewCount() const;

	// SET:
	void setPlaybackEnabled(const bool value);
	void resizeFunctionView(const size_t size);

	void resetPlayback();
	void setGlobalPlaybackSpeed( const double value );
	void setPlaybackTime( const double value );
	void setStatistics(
			const Statistics& statistics
	);

	// Actions:
		void focusFunction(const uint index);
		void addFunction();
		void removeFunction();
		void togglePlaybackEnabled();

signals:
	void functionCountChanged(const uint count);
	void globalPlaybackSpeedChanged(const double playbackSpeed);
	void isAudioEnabledChanged(const bool value);

private:
	Ui::MainWindow *ui;
	QLabel* statsDisplay;
	TipsDialog* tipsDialog;
	HelpDialog* helpDialog;
	StatisticsDialog* statsDialog;
	std::vector<FunctionView*> functionViews;
	
	// Data:
	double globalPlaybackSpeed;
};

#endif // MAINWINDOW_H
