#ifndef JACK_H
#define JACK_H

#include "fge/audio/audio_worker.h"
#include "fge/shared/data.h"
#include <jack/jack.h>


class JackClient {
	public:
		JackClient(
				const QString& clientName
		);
		virtual ~JackClient();

		// ***************************
		// control audio processing:
		// ***************************

		// start processing audio:
		MaybeError start(
				const Callbacks& callbacks
		);
		// stop processing audio:
		void stop();

		bool isRunning() const;

		QString getClientName() const;
		uint getSamplerate();
		const Statistics& getStatistics() const;

		void setBufferSize(
				const uint32_t size
		);

	friend int processAudio(
			jack_nframes_t nframes,
			void* arg
	);

	private:
		// init/exit jack client:
		MaybeError init();
		void exit();

	private:
		AudioWorker audioWorker;
		const QString clientName;
		uint bufferSize = 0;
		uint samplerate = 0;
		// jack:
		jack_client_t* client = nullptr;
		jack_port_t* ports[1] = { nullptr };

#ifdef AUDIO_STUB
		std::atomic<bool> isJackFakeThreadRunning;
		std::thread jackFakeThread;
#endif
};

#endif
