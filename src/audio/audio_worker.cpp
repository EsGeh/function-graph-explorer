#include "fge/audio/audio_worker.h"
#include <chrono>
#include <climits>
#include <cstddef>
#include <jack/jack.h>
#include <jack/types.h>
#include <QDebug>
#include <thread>
#ifdef __gnu_linux__
#include <pthread.h>
#endif

/********************
 * AudioWorker
*********************/

void AudioWorker::init(
		const Callbacks& callbacks,
		const uint size,
		const uint samplerate
)
{
	this->callbacks = callbacks;
	resize(size, samplerate);
}

void AudioWorker::resize(
		const uint size,
		const uint samplerate
)
{
	qDebug() << "buffer size:" << size;
	this->samplerate = samplerate;
	this->ringBuffer.init( size );
	using namespace std::chrono_literals;
	statistics.deadline = 1000000us / samplerate * size;
}

using namespace std::chrono_literals;

void AudioWorker::run() {
	position = 0;
	stopWorkerSignal = false;
	isRunning = true;
	using microsec = std::chrono::microseconds;
	statistics.avg_time = 0s;
	statistics.max_time = 0s;
	// repeatedly fill buffer:
	worker = std::thread([this]{
		while(!stopWorkerSignal) {
			const auto t0{std::chrono::steady_clock::now()};
			// wait, until buffer not being full,
			// then fill next window:
			ringBuffer.write([this](auto buffer){
				const auto t0{std::chrono::steady_clock::now()};
				callbacks.valuesToBuffer(
						buffer,
						position,
						samplerate
				);
				this->position += ringBuffer.getSize();
				const auto t1{std::chrono::steady_clock::now()};
				const auto diff = std::chrono::duration_cast<microsec>(t1 - t0);
				statistics.avg_time = diff;
				statistics.max_time = std::max( statistics.max_time, diff );
			});

			callbacks.betweenAudioCallback(position, samplerate);
		}
		qDebug().nospace() << "AUDIO THREAD done: ";
		isRunning = false;
	});
#ifdef __gnu_linux__
	pthread_setname_np( worker.native_handle(), "AUDIO WORKER" );
#endif
	qDebug().nospace() << "AUDIO THREAD: " << to_qstring( worker.get_id() );
}

void AudioWorker::stop() {
	stopWorkerSignal = true;
	worker.join();
	statistics.avg_time = 0s;
	statistics.max_time = 0s;
}

bool AudioWorker::getIsRunning() const {
	return isRunning;
}


const Statistics& AudioWorker::getStatistics() const {
	return statistics;
}
