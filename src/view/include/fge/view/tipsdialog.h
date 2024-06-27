#ifndef TIPSDIALOG_H
#define TIPSDIALOG_H

#include <QDialog>

namespace Ui {
class TipsDialog;
}

class TipsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TipsDialog(QWidget *parent = nullptr);
    ~TipsDialog();

private:
		void nextTip();
private:
    Ui::TipsDialog *ui;
		uint currentTip;
};

#endif // TIPSDIALOG_H
