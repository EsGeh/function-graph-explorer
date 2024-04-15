#include "model/model.h"
#include "view/mainwindow.h"
#include "view/jack.h"
#include "controller.h"
#include "config.h"

#include <QApplication>
#include <QDebug>


const unsigned int cacheResolution = 44100;
const unsigned int enableInterpolate = true;
const unsigned int viewResolution = 441;

int main(int argc, char *argv[])
{
	#ifndef NDEBUG
	qInfo() << "DEBUG configuration!\n";
	qInfo().nospace() << "-----------------------------";
	qInfo().nospace() << PROJECT_NAME << " " << PROJECT_VERSION;
	qInfo().nospace() << "-----------------------------";
	#endif

	JackClient jack;
	auto maybeError = jack.init();
	if( maybeError.has_value() ) {
		qCritical() << maybeError.value() ;
		return 1;
	}

	jack.startWorkerThread();

  QApplication a(argc, argv);
  auto model = Model(
			cacheResolution,
			enableInterpolate
	);
  auto view = MainWindow();
	Controller controller(
			&model,
			&view,
			&jack,
			viewResolution
	);
	controller.run();

  auto ret = a.exec();

	#ifndef NDEBUG
	qInfo() << "Exiting!\n";
	#endif

	jack.exit();

	return ret;
}
