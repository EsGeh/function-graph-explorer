#include "mainwindow.h"
#include "model.h"
#include "controller.h"

#include <QApplication>


int main(int argc, char *argv[])
{
	#ifndef NDEBUG
	qInfo() << "DEBUG configuration!\n";
	#endif
  QApplication a(argc, argv);
  Model model;
  MainWindow view;
  Controller controller(&model, &view);
  controller.run();
  return a.exec();
}
