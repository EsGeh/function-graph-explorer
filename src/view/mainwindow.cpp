#include "fge/view/mainwindow.h"
#include "include/fge/view/statistics.h"
#include "include/fge/view/tipsdialog.h"
#include "ui_mainwindow.h"
#include <QChart>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <qmainwindow.h>
#include <qnamespace.h>
#include <qspinbox.h>

using namespace std::chrono_literals;

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
	statsDisplay = new QLabel();
	ui->statusBar->addPermanentWidget( statsDisplay );
	tipsDialog = new TipsDialog(
			&resources->tips,
			this
	);
	helpDialog = new HelpDialog(this);
	statsDialog = new StatisticsDialog(this);
	connect(
		ui->functionCount,
		&QSpinBox::valueChanged,
		[this]( int value ) {
			emit functionCountChanged( value );
		}
	);
	connect(
		ui->audioEnabled,
		&QCheckBox::checkStateChanged,
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
		{
			auto action = helpMenu->addAction( "Statistics" );
			connect( action, &QAction::triggered,
					[this]() {
						statsDialog->show();
					}
			);
		}
	}
	this->setStatistics( Statistics{ 0us, 0us, 1us } );
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

void MainWindow::setPlaybackEnabled(const bool value)
{
	ui->audioEnabled->setChecked(value);
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

void MainWindow::setStatistics(
		const Statistics& statistics
)
{
	statsDialog->set( statistics );
	statsDisplay->setText( QString("CPU: %1%, Peak: %2%")
		.arg(
			QString::number(
				statistics.avg_time.count() * 100
				/ statistics.deadline.count()
			)
			// 2
		)
		.arg(
			QString::number(
				statistics.max_time.count() * 100
				/ statistics.deadline.count()
			)
			// 2
		)
	);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	// qDebug() << "MainWindow: key press" << event->key() ;
	if( event->modifiers() & Qt::ControlModifier ) {
		// ^1 ... ^9: select f1 ... f9
		if( event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9 ) {
			uint index = event->key() - Qt::Key_1;
			if( index < getFunctionViewCount() ) {
				auto functionView = getFunctionView( index );
				functionView->setFocus();
				ui->scrollArea->ensureWidgetVisible( functionView );
				return;
			}
		}
		// ^+ / ^-: add / remove function:
		if( event->key() == Qt::Key_Plus ) {
			ui->functionCount->setValue( ui->functionCount->value()+1 );
			return;
		}
		if( event->key() == Qt::Key_Minus ) {
			ui->functionCount->setValue( ui->functionCount->value()-1 );
			return;
		}
		// ^_: start / stop playback
		if( event->key() == Qt::Key_Space ) {
			ui->audioEnabled->toggle();
			return;
		}
	}
	QMainWindow::keyPressEvent( event );
}

