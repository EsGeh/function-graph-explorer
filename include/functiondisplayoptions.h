#ifndef FUNCTIONDISPLAYOPTIONS_H
#define FUNCTIONDISPLAYOPTIONS_H

#include "global.h"
#include <QDialog>


namespace Ui {
class FunctionDisplayOptions;
}

class FunctionDisplayOptions : public QDialog
{
    Q_OBJECT

public:
	explicit FunctionDisplayOptions(QWidget *parent = nullptr);
	~FunctionDisplayOptions();

	QString getFormula() const;
	std::pair<T,T> getOrigin() const;
	std::pair<T,T> getScale() const;
	T getPlaybackDuration() const;
	T getPlaybackSpeed() const;
	T getPlaybackOffset() const;

	void setFormula( const QString& value );
	void setOrigin( const std::pair<T,T>& value );
	void setScale( const std::pair<T,T>& value );
	void setPlaybackDuration( const T value );
	void setPlaybackSpeed( const T value );
	void setPlaybackOffset( const T value );

private:
    Ui::FunctionDisplayOptions *ui;
};

#endif // FUNCTIONDISPLAYOPTIONS_H
