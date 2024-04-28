#pragma once

#include "fge/shared/data.h"
#include <QWidget>

namespace Ui {
class ParameterValueEntry;
}

class ParameterValueEntry : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterValueEntry(
				const QString& name,
				const C& param,
				QWidget *parent = nullptr
		);
    ~ParameterValueEntry();

signals:
		void valueChanged(const C& value);

private:
    Ui::ParameterValueEntry *ui;
};
