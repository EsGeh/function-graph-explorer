#ifndef FUNCTIONDISPLAYOPTIONS_H
#define FUNCTIONDISPLAYOPTIONS_H

#include <QDialog>

namespace Ui {
class FunctionDisplayOptions;
}

typedef float T;

class FunctionDisplayOptions : public QDialog
{
    Q_OBJECT

public:
	explicit FunctionDisplayOptions(QWidget *parent = nullptr);
	~FunctionDisplayOptions();

	QString getFormula() const;
	std::pair<T,T> getOrigin() const;
	std::pair<T,T> getScale() const;

	void setFormula( const QString& value );
	void setOrigin( const std::pair<T,T>& value );
	void setScale( const std::pair<T,T>& value );

private:
    Ui::FunctionDisplayOptions *ui;
};

#endif // FUNCTIONDISPLAYOPTIONS_H
