#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "fge/model/model.h"
#include "fge/view/mainwindow.h"
#include "fge/audio/jack.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <concepts>
#include <mutex>
#include <qnamespace.h>
#include <ranges>
#include <utility>



/**
 * Mediator between GUI thread
 * and the Model.
 * Realizes decoupling the GUI
 * from the model wrt concurrency
 * by keeping a queue of updates.
 *
 * Explanation:
 * Since changes to the model may
 * take some time (due to audio ramping),
 * in order for the gui not being
 * blocked, the `ModelWorker`s update
 * slots immediately return and queue
 * up the model update.
 * A signal is sent after the update
 * has actually taken place.
*/
class ModelWorker: public QObject
{
	Q_OBJECT

public:
	explicit ModelWorker(
			Model* model,
			QObject *parent = nullptr
	)
		: model(model)
	{
		workerThread = new QThread(parent);
		workerThread->setObjectName("MODEL UPDATE QUEUE");
		timer = new QTimer(this);

		connect(workerThread, &QThread::finished, this, &QObject::deleteLater);
		connect(timer, &QTimer::timeout, this, &ModelWorker::tick);

		this->moveToThread( workerThread );
		workerThread->start();
		QMetaObject::invokeMethod(this, &ModelWorker::printThreadId, Qt::QueuedConnection );
	};

	~ModelWorker() {
		delete timer;
	}

signals:
	void tick();
	void writeDone(
			std::function<void()> doneCallback
	);

public:

	void exit() {
		timer->stop();
		workerThread->quit();
		workerThread->wait();
	}

	void printThreadId() {
		qDebug() << "GUI WORKER THREAD:" << (unsigned long )QThread::currentThreadId();
	}

	void startTimer() {
		QMetaObject::invokeMethod(this, [this]{
#ifdef DEBUG_CONCURRENCY
		timer->start(1000);
#else
		timer->start(10);
#endif
		}, Qt::QueuedConnection );
	}

	void stopTimer() {
		QMetaObject::invokeMethod(this, [this]{
			timer->stop();
		}, Qt::QueuedConnection );
	}

	template <typename Function, typename Continuation>
	void read(
			QObject* continueCtxt,
			Function f,
			Continuation doneCallback
	) {
		if constexpr ( std::same_as<decltype(f(std::as_const(model))),void> ) {
			QMetaObject::invokeMethod(
					this,
					[this,continueCtxt,f,doneCallback](){
						f(std::as_const(model));
						writeDone(
								[this,doneCallback]{
									doneCallback(std::as_const(model));
								}
						);
					},
					Qt::QueuedConnection
			);
		}
		else {
			QMetaObject::invokeMethod(
					this,
					[this,continueCtxt,f,doneCallback](){
						auto ret = f(std::as_const(model));
						writeDone(
								[this,doneCallback,ret]{
									doneCallback(std::as_const(model), ret);
								}
						);
					},
					Qt::QueuedConnection
			);
		}
	}

	template <typename Function, typename Continuation>
	void write(
			QObject* continueCtxt,
			const QString& updateName,
			Function f,
			Continuation doneCallback
	) {
		qDebug() << "ModelWorker::write" << updateName;
		if constexpr ( std::same_as<decltype(f(model)),void> ) {
			QMetaObject::invokeMethod(
					this,
					[this,continueCtxt,f,doneCallback]{
						f(model);
						emit writeDone(
								[this,doneCallback]{
									doneCallback(std::as_const(model));
								}
						);
					},
					Qt::QueuedConnection
			);
		}
		else {
			QMetaObject::invokeMethod(
					this,
					[this,continueCtxt,f,doneCallback]{
						auto ret = f(model);
						emit writeDone(
								[this,doneCallback,ret]{
									doneCallback(std::as_const(model), ret);
								}
						);
					},
					Qt::QueuedConnection
			);
		}
	}

private:
	Model* model;
	QThread* workerThread;
	QTimer* timer;
};

class Controller : public QObject
{
	Q_OBJECT

public:
	Controller(
		Model* model,
		MainWindow* view,
		JackClient* jack,
		const uint viewResolution
	);
	~Controller();
	void run();
	void exit();

public slots:
	void writeDoneCallback(
			std::function<void()> doneCallback
	);

private:
	void resizeView(
			const Model* model,
			const uint oldSize
	);

	void setViewFormula( const Model* model, const uint iFunction);
	void setViewGraph( const Model* model, const uint iFunction);

	void startPlayback();
	std::future<void> stopPlayback();

private:
	MainWindow* view;
	JackClient* jack;
	const uint viewResolution;
	ModelWorker* modelWorker;

};

#endif // CONTROLLER_H
