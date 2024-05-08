#include "fge/model/model.h"
#include "fge/view/mainwindow.h"
#include "controller.h"
#include "fge/audio/jack.h"
#include "fge/shared/config.h"

#include <QApplication>
#include <QDebug>
#include <thread>


const auto defSamplingSettings = SamplingSettings{
	.resolution = 44100,
	.interpolation = 1,
	.caching = true
};
const unsigned int viewResolution = 4410;

int main(int argc, char *argv[])
{
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

	qInfo().nospace() << "stop audio...";
	if( jack.isRunning() ) {
		jack.stop();
	}
	jack.exit();

	qInfo().nospace() << "exit.";
	return ret;
}
