#include "fge/view/tipsdialog.h"
#include "ui_tipsdialog.h"

#include <QPushButton>
#include <QScreen>
#include <cstdlib>
#include <qabstractbutton.h>
#include <qguiapplication.h>


TipsDialog::TipsDialog(
		const std::vector<QString>* tips,
		QWidget *parent
)
	: QDialog(parent)
	, ui(new Ui::TipsDialog)
	, tips( tips )
	, currentTip( 0 )
{
	currentTip = std::rand() % tips->size();
	ui->setupUi(this);
	connect(
		ui->closeBtn,
		&QAbstractButton::clicked,
		[this]{
			this->accept();
		}
	);
	connect(
		ui->nextTipBtn,
		&QAbstractButton::clicked,
		[this]{
			this->nextTip();
		}
	);
	nextTip();
	auto screenSize = QGuiApplication::screenAt(QPoint(0,0))->
		availableGeometry().size();
	setGeometry(
			QRect(
				QPoint(
					(screenSize / 2).width() - width()/2,
					(screenSize / 2).height() - height()/2
				),
				QSize( width(), height() )
			)
	);
}

TipsDialog::~TipsDialog()
{
	delete ui;
}

void TipsDialog::nextTip()
{
	currentTip++;
	currentTip %= tips->size();
	ui->textEdit->setMarkdown(
			tips->at(currentTip)
	);
}
