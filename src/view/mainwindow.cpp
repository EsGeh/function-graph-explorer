#include "view/mainwindow.h"
#include "./ui_mainwindow.h"
#include <QChart>
#include <QDoubleSpinBox>
#include <algorithm>


MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent)
		, ui(new Ui::MainWindow)
		, functionViews()
{
	ui->setupUi(this);
	connect(
		ui->functionCount,
		&QSpinBox::valueChanged,
		[this]( int value ) {
			emit functionCountChanged( value );
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
			/*
			qDebug() << funcView->objectName();
			qDebug() << funcView->sizePolicy();
			qDebug() << funcView->sizeHint();
			qDebug() << funcView->minimumSizeHint();
			*/
			functionViews.push_back(
					funcView
			);
		}
	}
}
