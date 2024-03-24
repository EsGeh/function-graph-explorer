#include "mainwindow.h"
#include "model.h"
#include "controller.h"

#include <QApplication>
#include <memory>


std::shared_ptr<Model> modelFactory();

void init(Model* model);

int main(int argc, char *argv[])
{
	#ifndef NDEBUG
	qInfo() << "DEBUG configuration!\n";
	#endif
  QApplication a(argc, argv);
  auto model = Model();
  auto view = MainWindow();
	Controller controller( &model, &view );
	controller.run();

  return a.exec();
}
