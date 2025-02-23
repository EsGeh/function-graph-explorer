#include "application.h"
#include <qnamespace.h>

Application::Application(int& argc, char **argv)
		: QApplication(argc, argv)
{
}

bool Application::notify(QObject *receiver, QEvent *event)
{
	if ( event->type() == QEvent::KeyPress ) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if(
				keyEvent->modifiers() & Qt::ControlModifier
				&& keyEvent->key() == Qt::Key_Space
		) {
			// qDebug() << "Notify method of Application" << keyEvent->text();
			// qDebug() << "dest" << receiver->objectName();
			emit togglePlaybackEnabled();
			return true;
		}
	}
	return QApplication::notify(receiver,event);
}
