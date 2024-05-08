#include "fge/audio/jack.h"
#include <jack/jack.h>
#include <jack/types.h>
#include <QDebug>
#include <thread>

// jack callback functions:

int processAudio(
		jack_nframes_t nframes,
		void* arg
);

int setBufferSizeCallback(
		jack_nframes_t nframes,
		void* arg
);

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
	this->samplerate = samplerate;
	for( uint i=0; i<count; i++ ) {
		buffer[i].resize(size);
	}
}

SampleTable* AudioWorker::getSamplesInit() {
	return &buffer[readIndex];
}

void AudioWorker::getSamplesExit() {
	readIndex = (readIndex + 1) % count;
	// signal the worker
	// thread, we are done
	lock.unlock();
}

void AudioWorker::run() {
	readIndex = 0;
	writeIndex = 0;
	position = 0;
	stopWorker = false;
	callbacks.startAudio();
	fillBuffer(writeIndex);
	callbacks.stopAudio();
	writeIndex = (writeIndex + 1) % count;
	// fill next buffer
	worker = std::thread([this]{
		while(!stopWorker) {
			lock.lock();
			callbacks.startAudio();
			fillBuffer(writeIndex);
			callbacks.stopAudio();
			writeIndex = (writeIndex + 1) % count;
			callbacks.betweenAudioCallback(position, samplerate);
		}
		qDebug().nospace() << "AUDIO THREAD done: ";
	});
	qDebug().nospace() << "AUDIO THREAD: " << to_qstring(worker.get_id());
}

void AudioWorker::stop() {
	stopWorker = true;
	lock.unlock();
	worker.join();
}

bool AudioWorker::isRunning() const {
	return !stopWorker;
}

void AudioWorker::fillBuffer(const uint index) {
	for(
			PlaybackPosition pos=0;
			pos<buffer[index].size();
			pos++
	) {
		buffer[index][pos] =
			callbacks.audioCallback(position+pos, samplerate);
		if( pos % 1 == 0 ) {
			callbacks.rampingCallback(position+pos, samplerate);
		}
	}
	position +=buffer[index].size();
}

/********************
 * JackClient
*********************/

JackClient::JackClient(
		const QString& clientName
)
	: clientName(clientName)
{}

JackClient::~JackClient()
{}

MaybeError JackClient::init() {
	try {
		{
			jack_status_t status;
			jack_options_t options = JackNullOption;
			client = jack_client_open(
					clientName.toStdString().c_str(),
					options,
					&status
			);
			if( !client ) {
				throw QString("error opening client");
			}
		}
		// set process callback:
		jack_set_process_callback(
				client,
				&processAudio,
				this
		);
		jack_set_buffer_size_callback(
				client,
				&setBufferSizeCallback,
				this
		);
		// create ports:
		{
			auto flags = JackPortIsOutput;
			{
				ports[0] = jack_port_register(
					client,
					"out",
					JACK_DEFAULT_AUDIO_TYPE,
					flags,
					0
				);
				if( !ports[0] ) {
					throw QString("error creating port");
				}
			}
		}

		samplerate = jack_get_sample_rate(client);
		if( jack_activate(client) ) {
			throw QString("failed to activate client");
		}
	}
	catch( const QString& error ) {
		client = nullptr;
		return { error };
	}

	return {};
}

void JackClient::exit() {
	if(!client) {
		return;
	}
	jack_client_close(
			client
	);
}

MaybeError JackClient::start(
		const Callbacks& callbacks
) {
	if( !client ) {
		return Error("No client!");
	}
	if( bufferSize == 0 ) {
		return "failed to determine buffer size";
	}
	audioWorker.init(
			callbacks,
			bufferSize,
			samplerate
	);
	audioWorker.run();
	return {};
}

void JackClient::stop() {
	if(!client) {
		return;
	}
	if( audioWorker.isRunning() ) {
		audioWorker.stop();
	}
}

bool JackClient::isRunning() const
{
	return audioWorker.isRunning();
}

QString JackClient::getClientName() const
{
	return clientName;
}

uint JackClient::getSamplerate()
{
	return samplerate;
}

int processAudio(
		jack_nframes_t nframes,
		void* arg
) {
	auto jackObj = (JackClient* )arg;
	sample_t* buffer = (sample_t* )jack_port_get_buffer(
			jackObj->ports[0],
			nframes
	);
	// clear buffer:
	if(
			!jackObj->audioWorker.isRunning()
	) {
		memset(buffer, 0, sizeof(sample_t) * nframes);
		return 0;
	}
	memcpy(
			buffer,
			jackObj->audioWorker.getSamplesInit()->data(),
			sizeof(sample_t) * nframes
	);
	jackObj->audioWorker.getSamplesExit();

	return 0;
}

int setBufferSizeCallback(
		jack_nframes_t nframes,
		void* arg
)
{
	auto jackObj = (JackClient* )arg;
	jackObj->bufferSize = nframes;
	return 0;
}
