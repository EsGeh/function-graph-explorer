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
    explicit TipsDialog(
				const std::vector<QString>* tips,
				QWidget *parent = nullptr
		);
    ~TipsDialog();

private:
		void nextTip();
private:
    Ui::TipsDialog *ui;
		const std::vector<QString>* tips;
		uint currentTip;
};

#endif // TIPSDIALOG_H
