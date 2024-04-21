#ifndef JACK_H
#define JACK_H

#include "fge/shared/utils.h"
#include <jack/jack.h>
#include <thread>


typedef jack_default_audio_sample_t sample_t;
typedef QString Error;

typedef std::vector<sample_t> SampleTable;

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
		// start processing audio:
		MaybeError run();
		// stop processing audio:
		void exit();

		// ***************************
		// play sample tables:
		// ***************************

		// writable pointer to
		// sampletable:
		SampleTable* getSampleTable();
		/* play sampletable.
		 * sets `playing = true`.
		 * sets `playing = false`, after
		 *   when done
		 */
		void play();
		// stop sampletable playback
		void stop();
		// return `playing`
		bool getIsPlaying();
		uint getPlayPos();

		// get:
		uint getSamplerate();

		// set:
		void setPlayPos(const uint value);

		jack_client_t* getClient();
		jack_port_t* getPort();
	protected:
	private:
		const QString clientName;
		// jack:
		jack_client_t* client = nullptr;
		jack_port_t* ports[1] = { nullptr };
		uint samplerate = 0;
		// sampletable
		bool playing = false;
		SampleTable sampleTable;
		uint playPos = 0;
		// worker thread:
		std::thread worker;
		bool workerStop = false;
};

#endif
