#include "jack.h"
#include <jack/types.h>
#include <QDebug>


const QString clientName = "fge";

const double pi = acos(-1);
const double freq = 440;


int processAudio(
		jack_nframes_t nframes,
		void* arg
);

JackClient::JackClient()
	: client( nullptr )
	, ports({ nullptr })
	, samplerate(0)
	, sampleTable()
	, offset(0)
	, worker()
	, workerStop(false)
{}

JackClient::~JackClient()
{}

SampleTable* JackClient::getSampleTable()
{
	return &sampleTable;
}

unsigned int JackClient::getSamplerate()
{
	return samplerate;
}

unsigned int& JackClient::getOffset()
{
	return offset;
}

jack_client_t* JackClient::getClient() {
	return client;
}

jack_port_t* JackClient::getPort() {
	return ports[0];
}

std::optional<Error> JackClient::init() {
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

void JackClient::exit() {
	qInfo() << "Stopping jack client" << clientName << "...";
	workerStop = true;
	worker.join();

	if(!client) {
		return;
	}
	jack_client_close(
			client
	);
}

void JackClient::startWorkerThread() {
	if( jack_activate(client) ) {
		throw QString("cannot activate client");
	}
	assert( client != nullptr);
	qInfo() << "Starting jack client" << clientName << "...";
	workerStop = false;
	worker = std::thread([this]{
			while(!workerStop) {
				// qDebug() << "tick";
				sleep(1);
			};
	});
}

void oscExample(
		SampleTable* table,
		unsigned int samplerate
)
{
	for( unsigned int i=0; i<table->size(); i++ ) {
		table->at(i) = sin( freq/samplerate * 2*pi * i );
	}
}

int processAudio(
		jack_nframes_t nframes,
		void* arg
) {
	auto jackObj = (JackClient* )arg;
	sample_t* table = jackObj->getSampleTable()->data();
	auto tableSize = jackObj->getSampleTable()->size();
	auto& offset = jackObj->getOffset();
	sample_t* buffer = (sample_t* )jack_port_get_buffer(
			jackObj->getPort(),
			nframes
	);
	jack_nframes_t frames_left = nframes;
	while (tableSize - offset < frames_left) {
		memcpy(
				buffer + (nframes - frames_left),
				table + offset,
				sizeof(sample_t) * (tableSize - offset)
		);
		frames_left -= tableSize - offset;
		offset = 0;
	}
	if (frames_left > 0) {
		memcpy(
				buffer + (nframes - frames_left),
				table + offset,
				sizeof(sample_t) * frames_left
		);
		offset += frames_left;
	}
	return 0;
}
