#include "fge/model/model.h"
#include "fge/view/mainwindow.h"
#include "fge/view/jack.h"
#include "controller.h"
#include "fge/shared/config.h"

#include <QApplication>
#include <QDebug>


const auto defSamplingSettings = SamplingSettings{
	.resolution = 44100,
	.interpolation = 1,
	.caching = true
};
const unsigned int viewResolution = 4410;

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
			defSamplingSettings
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
