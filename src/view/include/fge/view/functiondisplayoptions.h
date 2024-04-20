#ifndef FUNCTIONDISPLAYOPTIONS_H
#define FUNCTIONDISPLAYOPTIONS_H

#include "fge/view/viewdata.h"
#include <QDialog>


namespace Ui {
class FunctionDisplayOptions;
}

class FunctionDisplayOptions : public QDialog
{
    Q_OBJECT

public:
	explicit FunctionDisplayOptions(
			const FunctionViewData& viewData,
			QWidget *parent = nullptr
	);
	~FunctionDisplayOptions();

	QString getFormula();
	const FunctionViewData& getViewData();

	void setFormula(const QString& value);
	void setViewData(const FunctionViewData& value);

private:
	void updateView();

private:
    Ui::FunctionDisplayOptions *ui;
		FunctionViewData viewData;
};

#endif // FUNCTIONDISPLAYOPTIONS_H
