#include "fge/view/helpdialog.h"
#include "ui_helpdialog.h"


HelpDialog::HelpDialog(
		const QString& help,
		QWidget *parent
)
    : QDialog(parent)
    , ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
		ui->textEdit->setMarkdown( help );
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
