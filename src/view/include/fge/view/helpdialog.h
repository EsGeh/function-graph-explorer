#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include "fge/view/data.h"

#include <QDialog>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(
				const QString& help,
				QWidget *parent = nullptr
		);
    ~HelpDialog();

private:
    Ui::HelpDialog *ui;
};

#endif // HELPDIALOG_H
