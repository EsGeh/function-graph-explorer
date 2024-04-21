#include "fge/audio/jack.h"
#include <jack/types.h>
#include <QDebug>


int processAudio(
		jack_nframes_t nframes,
		void* arg
);

JackClient::JackClient(
		const QString& clientName
)
	: clientName(clientName)
{}

JackClient::~JackClient()
{}

SampleTable* JackClient::getSampleTable()
{
	return &sampleTable;
}

uint JackClient::getSamplerate()
{
	return samplerate;
}

bool JackClient::getIsPlaying() {
	return playing;
}

uint JackClient::getPlayPos() {
	return playPos;
}

jack_client_t* JackClient::getClient() {
	return client;
}

jack_port_t* JackClient::getPort() {
	return ports[0];
}

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
		// create ports:
		{
			auto flags = JackPortIsOutput;
			{
				ports[0] = jack_port_register(
					client,
					"out_left",
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
		// qInfo() << "samplerate: " << samplerate;
		sampleTable.resize( samplerate * 1, 0 );
	}
	catch( const QString& error ) {
		client = nullptr;
		// table = nullptr;
		return { error };
	}

	return {};
}

MaybeError JackClient::run() {
	if( client == nullptr ) {
		return Error("Client not initialized. call 'JackClient::init()' first!");
	}
	if( jack_activate(client) ) {
		return "cannot activate client";
	}
	qInfo() << "Starting jack client" << clientName << "...";
	workerStop = false;
	worker = std::thread([this]{
			while(!workerStop) {
				// qDebug() << "tick";
				sleep(1);
			};
	});
	return {};
}

void JackClient::exit() {
	if(!client) {
		return;
	}
	qInfo() << "Stopping jack client" << clientName << "...";
	workerStop = true;
	worker.join();
	jack_client_close(
			client
	);
}

void JackClient::play() {
	playing = true;
}

void JackClient::stop() {
	playing = false;
	playPos = 0;
}

void JackClient::setPlayPos(const uint value) {
	playPos = value;
}

int processAudio(
		jack_nframes_t nframes,
		void* arg
) {
	auto jackObj = (JackClient* )arg;
	sample_t* table = jackObj->getSampleTable()->data();
	auto tableSize = jackObj->getSampleTable()->size();
	sample_t* buffer = (sample_t* )jack_port_get_buffer(
			jackObj->getPort(),
			nframes
	);
	if( !jackObj->getIsPlaying() ) {
		memset(buffer, 0, sizeof(sample_t) * nframes);
		return 0;
	}
	const uint samplesLeft =
		tableSize - jackObj->getPlayPos();
	// if we still have enough samples
	// to fill the buffer completely:
	if( nframes <= samplesLeft ) {
		memcpy(
				buffer,
				table + jackObj->getPlayPos(),
				sizeof(sample_t) * nframes
		);
		jackObj->setPlayPos( jackObj->getPlayPos() + nframes );
	}
	// if we don't have enough
	// samples left to fill the
	// buffer:
	else {
		// copy the rest of the table
		memcpy(
				buffer,
				table + jackObj->getPlayPos(),
				sizeof(sample_t) * samplesLeft
		);
		// ...and fill the rest of the
		// buffer with zeros:
		memset(
				buffer+samplesLeft,
				0,
				sizeof(sample_t) * (nframes-samplesLeft)
		);
		jackObj->stop();
	}

	return 0;
}
