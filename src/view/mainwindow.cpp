#include "fge/view/mainwindow.h"
#include "ui_mainwindow.h"
#include <QChart>
#include <QDoubleSpinBox>
#include <QCheckBox>


MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent)
		, ui(new Ui::MainWindow)
		, functionViews()
{
	ui->setupUi(this);
	ui->time->setEnabled(false);
	connect(
		ui->functionCount,
		&QSpinBox::valueChanged,
		[this]( int value ) {
			emit functionCountChanged( value );
		}
	);
	connect(
		ui->audioEnabled,
		&QCheckBox::stateChanged,
		[this]( auto value ) {
			emit isAudioEnabledChanged( value != 0 );
		}
	);
}

MainWindow::~MainWindow()
{
	delete ui;
}

FunctionView* MainWindow::getFunctionView(const size_t index) const {
	return functionViews.at( index );
}

size_t MainWindow::getFunctionViewCount() const {
	return ui->functionCount->value();
}

void MainWindow::resizeFunctionView(const size_t size) {
	if( functionViews.size() > size ) {
		while( functionViews.size() > size ) {
			auto last = functionViews.back();
			ui->verticalLayout->removeWidget( last );
			last->deleteLater();
			functionViews.pop_back();
		}
	}
	else if( functionViews.size() < size ) {
		while( functionViews.size() < size ) {
			auto funcView = new FunctionView(
					QString("f%1(x) =").arg(functionViews.size()) // title
			);
			// insert directly before the final spacer:
			ui->verticalLayout->insertWidget(
					ui->verticalLayout->count()-1,
					funcView,
					1
			);
			functionViews.push_back(
					funcView
			);
		}
	}
}

void MainWindow::resetPlayback()
{
	for( auto funcView : functionViews ) {
		funcView->disablePlaybackPosition();
	}
}

void MainWindow::setPlaybackTime( const double value )
{
	auto blockedOld = ui->time->blockSignals(true);
	ui->time->setValue( value );
	ui->time->blockSignals( blockedOld );
	for( auto funcView : functionViews ) {
		if( !funcView->getIsPlaybackEnabled() ) {
			continue;
		}
		funcView->setPlaybackTime( value );
	}
}
