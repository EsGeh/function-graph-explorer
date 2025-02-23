#pragma once

#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

/**
 * Some keycodes are not corectly propagated
 * through event gui widget handlers: Qt::Key_Space.
 * Therefore we catch it in the QApplication
 * instance. That's why this class is needed
 */
class Application: public QApplication
{
	Q_OBJECT
public:
	explicit Application(int& argc, char **argv);

signals:
	void togglePlaybackEnabled();
protected:
	bool notify(QObject *receiver, QEvent *event) override;
};
