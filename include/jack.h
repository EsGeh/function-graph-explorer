#ifndef JACK_H
#define JACK_H

#include <jack/jack.h>
#include <QString>

#include <thread>
#include <optional>


typedef jack_default_audio_sample_t sample_t;
typedef QString Error;

typedef std::vector<sample_t> SampleTable;

class JackClient {
	public:
		JackClient();
		virtual ~JackClient();

		// get:
		SampleTable* getSampleTable();
		unsigned int getSamplerate();
		bool getIsPlaying();
		unsigned int getPlayPos();
		jack_client_t* getClient();
		jack_port_t* getPort();
		
		// set:
		std::optional<Error> init();
		void exit();
		void startWorkerThread();
		void play();
		void stop();
		void setPlayPos(const unsigned int value);
	private:
		// jack:
		jack_client_t* client;
		jack_port_t* ports[1];
		unsigned int samplerate = 0;
		// sampletable
		bool playing;
		SampleTable sampleTable;
		unsigned int playPos;
		// worker thread:
		std::thread worker;
		bool workerStop;
};

#endif
