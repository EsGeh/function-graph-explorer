#ifndef JACK_H
#define JACK_H

#include "fge/shared/utils.h"
#include <jack/jack.h>
#include <thread>
#include <QDebug>


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
Work a little bit ahead 
to ensure, that there is
always audio data for
jack to play.
Prefills [an] audio buffer[s]
by invoking a callback.
*/

class AudioWorker
{
	const static uint count = 2;
	public:
		void init(
				const Callbacks& callbacks,
				const uint size,
				const uint samplerate
		);

		// Get the (hopefully)
		// already prefilled buffer.
		// Always call `getSamplesExit()`
		// after done with the buffer,
		// so that the worker may
		// continue.
		SampleTable* getSamplesInit();

		// Signalise we are done
		// with the current buffer
		// so the worker thread
		// may continue.
		void getSamplesExit();

		// Start worker thread
		void run();
		// Stop worker thread
		void stop();

		// Is worker thread running?
		bool isRunning() const;
	private:
		void fillBuffer(const uint index);
	private:
		// worker thread:
		std::mutex lock;
		std::thread worker;
		bool stopWorker = true;
		// buffers:
		SampleTable buffer[count];
		uint readIndex = 0;
		uint writeIndex = 0;
		// audio callback:
		Callbacks callbacks;
		PlaybackPosition position = 0;
		uint samplerate = 0;
};


class JackClient {
	public:
		JackClient(
				const QString& clientName = "fge"
		);
		virtual ~JackClient();

		// ***************************
		// control audio processing:
		// ***************************

		// initialize jack client:
		MaybeError init();
		void exit();

		// start processing audio:
		MaybeError start(
				const Callbacks& callbacks
		);
		// stop processing audio:
		void stop();

		bool isRunning() const;

		QString getClientName() const;
		uint getSamplerate();

	friend int processAudio(
			jack_nframes_t nframes,
			void* arg
	);

	friend int setBufferSizeCallback(
			jack_nframes_t nframes,
			void* arg
	);
	private:
		AudioWorker audioWorker;
		const QString clientName;
		uint bufferSize = 0;
		uint samplerate = 0;
		bool playing = false;
		// jack:
		jack_client_t* client = nullptr;
		jack_port_t* ports[1] = { nullptr };
		// AudioCallback callback = nullptr;
		// PlaybackPosition position = 0;
		// worker thread:
		std::thread worker;
		bool workerStop = false;
};

#endif
