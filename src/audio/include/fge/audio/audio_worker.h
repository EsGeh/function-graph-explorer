#ifndef AUDIO_WORKER_H
#define AUDIO_WORKER_H

#include "fge/audio/sample_ring_buffer.h"
#include "fge/shared/utils.h"
#include <chrono>
#include <jack/jack.h>
#include <semaphore>
#include <thread>
#include <QDebug>

#ifdef DEBUG_CONCURRENCY
#define AUDIO_STUB
#endif


typedef jack_default_audio_sample_t sample_t;
typedef QString Error;

using PlaybackPosition = unsigned long int;

using SampleTable = std::vector<sample_t>;

using AudioCallback =
	std::function<void(std::vector<float>* buffer,const PlaybackPosition position, const uint samplerate)>;

using SchedulerCallback =
	std::function<void(PlaybackPosition position,uint samplerate)>;

using SignalCallback = std::function<void()>;

struct Callbacks {
	SignalCallback startAudio, stopAudio;
	AudioCallback valuesToBuffer;
	SchedulerCallback betweenAudioCallback;
};

/**
 * Work a little bit ahead 
 * and fill a ring buffer
 * with audio data to ensure
 * that there is always
 * audio data for jack to play.
 */
class AudioWorker
{
	public:
		void init(
				const Callbacks& callbacks,
				const uint size,
				const uint samplerate
		);
		void resize(
				const uint size,
				const uint samplerate
		);

		void fillBuffer(
				sample_t* buffer
		)
		{
			ringBuffer.read([buffer, size=ringBuffer.getSize()](auto srcBuffer) {
				memcpy(
						buffer,
						srcBuffer->data(),
						sizeof(sample_t) * size
				);
			});
		}

		// Start worker thread
		void run();
		// Stop worker thread
		void stop();

		// Is worker thread running?
		bool getIsRunning() const;
		const Statistics& getStatistics() const;
	private:
		SampleRingBuffer ringBuffer;
		std::thread worker;
		std::atomic<bool> stopWorkerSignal = true;
		std::atomic<bool> isRunning = false;
		// audio callback:
		Callbacks callbacks;
		PlaybackPosition position = 0;
		uint samplerate = 0;

		Statistics statistics;
};

#endif
