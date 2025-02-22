#ifndef STATISTICS_H
#define STATISTICS_H

#include <QDialog>
#include "fge/shared/data.h"

namespace Ui {
class StatisticsDialog;
}

class StatisticsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit StatisticsDialog(QWidget *parent = nullptr);
	~StatisticsDialog();
	
	void set(
			const Statistics& statistics
	);

private:
	Ui::StatisticsDialog *ui;
};

#endif // STATISTICS_H
