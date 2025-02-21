#include "fge/audio/jack.h"
#include <jack/jack.h>
#include <jack/types.h>
#include <QDebug>
#include <thread>
#ifdef __gnu_linux__
#include <pthread.h>
#endif

#ifdef DEBUG_CONCURRENCY
#include <chrono>
using namespace std::chrono_literals;
#endif

/********************
 * jack callback function
*********************/

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
}

void AudioWorker::run() {
	position = 0;
	stopWorkerSignal = false;
	isRunning = true;
	// repeatedly fill buffer:
	worker = std::thread([this]{
		while(!stopWorkerSignal) {
			ringBuffer.write([this](auto buffer){
				callbacks.valuesToBuffer(
						buffer,
						position,
						samplerate
				);
				this->position += ringBuffer.getSize();
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
}

bool AudioWorker::getIsRunning() const {
	return isRunning;
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
#ifndef AUDIO_STUB
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
#else
	bufferSize = 1024;
	samplerate = 44100;
#endif

	return {};
}

void JackClient::exit() {
#ifndef AUDIO_STUB
	if(!client) {
		return;
	}
	jack_client_close(
			client
	);
#endif
}

MaybeError JackClient::start(
		const Callbacks& callbacks
) {
#ifndef AUDIO_STUB
	if( !client ) {
		return Error("No client!");
	}
#endif
	if( bufferSize == 0 ) {
		return "failed to determine buffer size";
	}
	audioWorker.init(
			callbacks,
			bufferSize,
			samplerate
	);
	audioWorker.run();
#ifdef AUDIO_STUB
	isJackFakeThreadRunning = true;
	jackFakeThread = std::thread([this]() {
		while(isJackFakeThreadRunning) {
			std::this_thread::sleep_for( 2s );
			audioWorker.getSamplesInit();
			audioWorker.getSamplesExit();
		};
	});
#endif
	return {};
}

void JackClient::stop() {
#ifndef AUDIO_STUB
	if(!client) {
		return;
	}
#endif
	if( audioWorker.getIsRunning() ) {
		audioWorker.stop();
	}
#ifdef AUDIO_STUB
	isJackFakeThreadRunning = false;
	jackFakeThread.join();
#endif
}

bool JackClient::isRunning() const
{
#ifndef AUDIO_STUB
	return audioWorker.getIsRunning();
#else
	return isJackFakeThreadRunning;
#endif
}

QString JackClient::getClientName() const
{
	return clientName;
}

uint JackClient::getSamplerate()
{
	return samplerate;
}

void JackClient::setBufferSize(
		const uint32_t size
)
{
	bufferSize = size;
	audioWorker.resize(
			size,
			samplerate
	);
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
			!jackObj->audioWorker.getIsRunning()
	) {
		memset(buffer, 0, sizeof(sample_t) * nframes);
		return 0;
	}
	jackObj->audioWorker.fillBuffer( buffer );
	return 0;
}

int setBufferSizeCallback(
		jack_nframes_t nframes,
		void* arg
)
{
	auto jackObj = (JackClient* )arg;
	jackObj->setBufferSize( nframes );
	return 0;
}
