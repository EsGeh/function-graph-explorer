#ifndef FUNCTIONVIEW_H
#define FUNCTIONVIEW_H

#include "fge/view/parameter_utils.h"
#include "fge/view/viewdata.h"
#include "fge/view/parametersedit.h"
#include "fge/view/graphview.h"
#include "fge/view/functiondisplayoptions.h"

#include <QWidget>
#include <QStatusBar>


namespace Ui {
class FunctionView;
}

class FunctionView : public QWidget
{
  Q_OBJECT

public:

		struct UpdateInfo {
			std::optional<QString> formula = {};
			std::optional<ParameterBindings> parameters = {};
			std::optional<StateDescriptions> stateDescriptions = {};
			std::optional<bool> playbackEnabled = {};
			std::optional<SamplingSettings> samplingSettings= {};
		};

public:
	explicit FunctionView(
			const QString& title,
			QWidget *parent = nullptr
	);
	~FunctionView();

	QString getFormula();
	const ParameterBindings& getParameters() const;
	StateDescriptions getStateDescriptions() const;
	const FunctionViewData& getViewData() const;
	const SamplingSettings& getSamplingSettings() const;

  void setFormula( const QString& str );
  void setParameters( const ParameterBindings& value );
	void setSamplingSettings(const SamplingSettings& value);

  void setGraph(
      const std::vector<std::pair<C,C>>& values
  );
  void setFormulaError( const QString& str );

signals:
	void changed( UpdateInfo updateInfo );
	void parameterChanged(
			const QString& parameterName,
			const std::vector<C> value
	);
  void viewParamsChanged();

private:
	// UI:
	Ui::FunctionView *ui;
  GraphView* graphView;
	ParametersEdit* parametersDialog;
	FunctionDisplayOptions* displayDialog;
	QStatusBar* statusBar;
	// Data:
	ParameterBindings parameters;

	FunctionDataDescription dataDescription;
	FunctionViewData viewData;
	SamplingSettings samplingSettings;
};

void updateParameters(
		const std::map<QString,ParameterDescription>& parameterDescription,
		ParameterBindings& parameters
);

#endif // FUNCTIONVIEW_H
