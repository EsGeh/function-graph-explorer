#include "fge/view/statistics.h"
#include "ui_statistics.h"

StatisticsDialog::StatisticsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StatisticsDialog)
{
    ui->setupUi(this);
}

StatisticsDialog::~StatisticsDialog()
{
    delete ui;
}

void StatisticsDialog::set(
		const Statistics& statistics
)
{
	ui->avgLabel->setText(QString::number(uint(
			statistics.avg_time.count() * 100
			/ statistics.deadline.count()
	)) + "%" );
	ui->maxLabel->setText( QString::number(uint(
				statistics.max_time.count() * 100
			/ statistics.deadline.count()
	)) + "%" );
	ui->deadlineLabel->setText( QString::number(statistics.deadline.count()/1000.0) + "ms" );
}
