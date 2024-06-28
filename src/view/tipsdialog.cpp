#include "fge/view/tipsdialog.h"
#include "ui_tipsdialog.h"

#include <QPushButton>
#include <QScreen>
#include <cstdlib>
#include <qabstractbutton.h>
#include <qguiapplication.h>


const std::vector<QString> tips = {
	(QStringList {
		"# Fourier Transform Reenactment Day",
		"",
		"Sometimes, a practical example sheds light on a complex mathematical issue.",
		"Why not reenact the Fourier Transform for educational purposes?",
		"Come, grab your friends!"
	}).join("\n"),
	(QStringList {
		"# Magic Frequencies",
		"",
		"Some people claim that specific frequences have a certain effect on the human mind.",
		"Why not give it a try?",
		"Be warned though: There are rumors that a sinusoid at 666Hz summons the devil.",
		"There is actually a competition (just google around - you will find it!) that awards a prize of 1 million dollars to the person who succeeds in finding the frequency that makes the devil appear.",
		"There is a debate around the questions of scientific value of such competitions - since, as it has been correctly pointed out, there is a hack:",
		"1. First summon the devil by whatever means",
		"2. Sign a contract with him that binds the devil to make you win the aforementioned competition"
	}).join("\n"),
	(QStringList {
		"# Sinusoids",
		"",
		"Linguists claim that the word \"Sinus\" etymologically is rooted in Latin and is supposed to mean a fold in a garment or figuratively sometimes refers to \"bosom\" or \"gulf\" in the sense of \"The Bosom of the Sea\". In Anatomy, it refers to an arc or cavity of the body, such as in \"Sinus Urogenitalis\".",
		"We should, of course, distance ourselves from these perverts."
	}).join("\n")
};

TipsDialog::TipsDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::TipsDialog)
	, currentTip( 0 )
{
	currentTip = std::rand() % tips.size();
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
	currentTip %= tips.size();
	ui->textEdit->setMarkdown(
			tips[currentTip]
	);
}
