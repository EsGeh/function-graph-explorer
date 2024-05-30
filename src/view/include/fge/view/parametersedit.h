#ifndef PARAMETERSEDIT_H
#define PARAMETERSEDIT_H

#include "fge/view/parametervalueentry.h"
#include "fge/shared/parameter_utils.h"
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
				const QString& functionName,
				ParameterBindings* parameters,
				std::map<QString,ParameterDescription>* parameterDescriptions,
				QWidget *parent = nullptr
		);
    ~ParametersEdit();

		void updateView();
signals:
	void parameterChanged(
			const QString& parameterName,
			const std::vector<C>& value
	);

private:
    Ui::ParametersEdit *ui;
		std::vector<std::shared_ptr<ParameterValueEntry>> paramWidgets;
		// Data:
		ParameterBindings* parameters;
		std::map<QString,ParameterDescription>* parameterDescriptions;
};

#endif // PARAMETERSEDIT_H
