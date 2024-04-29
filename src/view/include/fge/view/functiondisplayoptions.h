#ifndef FUNCTIONDISPLAYOPTIONS_H
#define FUNCTIONDISPLAYOPTIONS_H

#include "fge/view/viewdata.h"
#include "fge/shared/data.h"
#include <QDialog>


namespace Ui {
class FunctionDisplayOptions;
}

class FunctionDisplayOptions : public QDialog
{
    Q_OBJECT

public:
	explicit FunctionDisplayOptions(
			const std::vector<QString>& parameters,
			const FunctionViewData& viewData,
			const SamplingSettings& samplingSettings,
			QWidget *parent = nullptr
	);
	~FunctionDisplayOptions();

	QString getFormula() const;
	std::vector<QString> getParameters() const;
	StateDescriptions getStateDescriptions() const;
	const FunctionViewData& getViewData() const;
	const SamplingSettings& getSamplingSettings() const;

	void setFormula(const QString& value);
	void setFunctionDataDescriptions(
			const std::vector<QString>& parameters,
			const StateDescriptions& state
	);
	void setViewData(const FunctionViewData& value);
	void setSamplingSettings(const SamplingSettings& value);

private:
	void updateView();

private:
    Ui::FunctionDisplayOptions *ui;
		FunctionViewData viewData;
		SamplingSettings samplingSettings;
};

#endif // FUNCTIONDISPLAYOPTIONS_H
