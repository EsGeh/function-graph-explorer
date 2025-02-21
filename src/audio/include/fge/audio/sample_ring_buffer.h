#ifndef SAMPLE_RING_BUFFER_H
#define SAMPLE_RING_BUFFER_H

#include <semaphore>
#include <vector>


typedef float sample_t;
using SampleTable = std::vector<sample_t>;


/**
 * ring buffer of count=2 sample tables
 */
class SampleRingBuffer
{
	public:
		const static uint count = 2;
		uint getSize() const { return size; }
		void init( const uint size )
		{
			this->size = size;
			for( uint i=0; i<count; i++ ) {
				buffer[i].resize(size);
			}
		}
		/// read from the buffer
		/// blocks if empty
		template <typename Function>
		void read( Function f)
		{
			hasData.acquire();
			f( &buffer[readIndex] );
			readIndex = (readIndex + 1) % count;
			notFull.release();
		}
		/// write to the buffer
		/// blocks if full
		template <typename Function>
		void write( Function f)
		{
			notFull.acquire();
			f( &buffer[writeIndex] );
			writeIndex = (writeIndex + 1) % count;
			hasData.release();
		}
	private:
		SampleTable buffer[count];
		uint size;
		uint readIndex = 0;
		uint writeIndex = 0;
		std::counting_semaphore<count> notFull{count};
		std::counting_semaphore<count> hasData{0};
};

#endif
