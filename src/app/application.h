#pragma once

#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

class Application: public QApplication
{
	Q_OBJECT
public:
	explicit Application(int& argc, char **argv);
};
