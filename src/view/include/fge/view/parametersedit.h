#ifndef PARAMETERSEDIT_H
#define PARAMETERSEDIT_H

#include "fge/view/parametervalueentry.h"
#include "fge/shared/data.h"
#include <QDialog>
#include <memory>

namespace Ui {
class ParametersEdit;
}

class ParametersEdit : public QDialog
{
    Q_OBJECT

public:
    explicit ParametersEdit(
				ParameterBindings* parameters,
				QWidget *parent = nullptr
		);
    ~ParametersEdit();

		void updateView();
signals:
	void parametersChanged();

private:
    Ui::ParametersEdit *ui;
		std::vector<std::shared_ptr<ParameterValueEntry>> paramWidgets;
		// Data:
		ParameterBindings* parameters;
};

#endif // PARAMETERSEDIT_H
