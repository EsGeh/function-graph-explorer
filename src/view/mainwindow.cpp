#include "fge/view/mainwindow.h"
#include "include/fge/view/tipsdialog.h"
#include "ui_mainwindow.h"
#include <QChart>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <qspinbox.h>


MainWindow::MainWindow(
		const Resources* resources,
		QWidget *parent
)
		: QMainWindow(parent)
		, ui(new Ui::MainWindow)
		, tipsDialog( nullptr )
		, helpDialog( nullptr )
		, functionViews()
{
	ui->setupUi(this);
	ui->time->setEnabled(false);
	tipsDialog = new TipsDialog(
			&resources->tips,
			this
	);
	helpDialog = new HelpDialog(this);
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
			if( value ) {
				ui->playbackSpeed->setEnabled( false );
			}
			else {
				ui->playbackSpeed->setEnabled( true );
			}
			emit isAudioEnabledChanged( value != 0 );
		}
	);
	connect(
		ui->playbackSpeed,
		&QDoubleSpinBox::valueChanged,
		[this]( auto value ) {
			globalPlaybackSpeed = value;
			emit globalPlaybackSpeedChanged( value );
		}
	);
	{
		auto cornerMenuBar = new QMenuBar(ui->menuBar);
		QMenu* helpMenu = cornerMenuBar->addMenu("Help");
		ui->menuBar->setCornerWidget( cornerMenuBar );
		{
			auto action = helpMenu->addAction( "Tips" );
			connect( action, &QAction::triggered,
					[this]() {
						tipsDialog->show();
					}
			);
		}
		{
			auto action = helpMenu->addAction( "Key Codes" );
			connect( action, &QAction::triggered,
					[this]() {
						helpDialog->show();
					}
			);
		}
	}
	tipsDialog->show();
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
					QString("f%1(x) =").arg(functionViews.size()), // title
					&globalPlaybackSpeed
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

void MainWindow::setGlobalPlaybackSpeed( const double value )
{
	auto blockedOld = ui->playbackSpeed->blockSignals(true);
	globalPlaybackSpeed = value;
	ui->playbackSpeed->setValue( value );
	ui->playbackSpeed->blockSignals( blockedOld );
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
