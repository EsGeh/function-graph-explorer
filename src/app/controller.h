#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "fge/model/model.h"
#include "fge/view/mainwindow.h"
#include "fge/audio/jack.h"

#include <QObject>
#include <QThread>

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
	{};

signals:
	void resizeDone(
			const Model* model,
			const uint oldSize
	);
	void updateDone(
			const Model* model,
			const uint index,
			const Model::Update& update,
			const MaybeError maybeError
	);
	void readAccessGranted( const Model* model, const uint index);

public slots:
	// queue model->resize
	// implicitly locks the model:
  void resize(
			const uint size
	) {
		modelLock.lock();
		auto oldSize = model->size();
		model->resize( size );
		model->postSetAny();
		emit resizeDone( model, oldSize );
	};

	// queue model->update
	// implicitly locks the model:
  void updateFunction(
			const uint index,
			const Model::Update& update
	) {
		modelLock.lock();
		if( index >= model->size() ) {
			qDebug() << "Skipping update. Entry no longer exists.";
			return;
		}
		auto ret = model->bulkUpdate(index, update );
		emit updateDone( model, index, update, ret );
	};

	// request read access to model:
	// implicitly locks the model:
	void requestRead(const uint index) {
		modelLock.lock();
		emit readAccessGranted( model, index );
	}

  void unlockModel() {
		modelLock.unlock();
	}

private:
	std::mutex modelLock;
	Model* model;

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

private:
	void resizeView(
			const Model* model,
			const uint oldSize
	);

	void setViewFormula( const Model* model, const uint iFunction);
	void setViewGraph( const Model* model, const uint iFunction);

private:
	MainWindow* view;
	JackClient* jack;
	const uint viewResolution;
	QThread* workerThread;
	ModelWorker* modelWorker;

};

#endif // CONTROLLER_H
