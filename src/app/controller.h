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
			const uint size,
			const uint oldSize
	);
	void updateDone(
			const uint index,
			const Model::Update& update,
			const MaybeError maybeError
	);

public slots:
  void resize(
			const uint size
	) {
		auto oldSize = model->size();
		model->resize( size );
		model->postSetAny();
		emit resizeDone( size, oldSize );
	};

  void updateFunction(
			const uint index,
			const Model::Update& update
	) {
		auto ret = model->bulkUpdate(index, update );
		emit updateDone( index, update, ret );
	};

private:
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
			const uint size,
			const uint oldSize
	);

	void setViewFormula(const uint iFunction);
	void setViewGraph( const uint iFunction);

private:
	Model* model;
	MainWindow* view;
	JackClient* jack;
	const uint viewResolution;
	QThread* workerThread;
	ModelWorker* modelWorker;
};

#endif // CONTROLLER_H
