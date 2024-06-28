#include "controller.h"
#include "resources.h"
#include "fge/model/model.h"
#include "fge/view/mainwindow.h"
#include "fge/audio/jack.h"
#include "fge/shared/config.h"

#include <QApplication>
#include <QDebug>
#include <thread>


const auto defSamplingSettings = SamplingSettings{
	.resolution = 44100,
	.interpolation = 1,
	.periodic = 0,
	.buffered = false
};
const unsigned int viewResolution = 4410;

int main(int argc, char *argv[])
{
	std::srand(std::time(0));
#ifndef NDEBUG
	qSetMessagePattern("[%{time hh:mm:ss}] [%{threadid}] %{message}");
#endif
	qInfo().nospace() << "-----------------------------";
	qInfo().nospace() << PROJECT_NAME << " " << PROJECT_VERSION;
	qInfo().nospace() << "-----------------------------";
	#ifndef NDEBUG
	qDebug().nospace() << "DEBUG configuration!";
	qDebug().nospace() << "MAIN THREAD / GUI THREAD: " << to_qstring(std::this_thread::get_id());
	#endif

	JackClient jack("fge");
	{
		qInfo().nospace() << "start jack client '" << jack.getClientName() << "'...";
		auto maybeError = jack.init();
		if( maybeError ) {
			qWarning().noquote() << "Failed to start Jack. No Audio.";
			qWarning().noquote() << maybeError.value() ;
		}
		qInfo().nospace() << "done";
	}

	QApplication a(argc, argv);
	auto model = modelFactory(
			defSamplingSettings
	);
	Resources resources = loadResources();
	auto view = MainWindow(
			&resources
	);
	Controller controller(
			model.get(),
			&view,
			&jack,
			viewResolution
	);
	controller.run();

	auto ret = a.exec();

	controller.exit();
	jack.exit();

	qInfo().nospace() << "exit.";
	return ret;
}
