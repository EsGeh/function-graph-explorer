#include "application.h"
#include <qnamespace.h>

Application::Application(int& argc, char **argv)
		: QApplication(argc, argv)
{
}

bool Application::notify(QObject *receiver, QEvent *event)
{
	return QApplication::notify(receiver,event);
}
