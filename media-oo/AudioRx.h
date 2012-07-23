
#ifndef __AUDIO_RX_H__
#define __AUDIO_RX_H__

extern "C" {
#include "libavformat/avformat.h"
}

#include "Media.h"
#include "util/Lock.h"

typedef struct DecodedAudioSamples {
	uint8_t *samples;
	int size;
	AVRational time_base;
	uint64_t pts;
	int64_t start_time;
	int64_t rx_time;
	int encoded_size;
	void *priv_data; /* User private data */
} DecodedAudioSamples;

typedef void (*put_audio_samples_rx)(DecodedAudioSamples* decoded_audio_samples);

namespace media {
	class AudioRx : public Media {
	private:
		const char *_sdp;
		int _max_delay;
		put_audio_samples_rx _callback;

		Lock *_mutex;
		Lock *_freeLock;
		int _receive;

	public:
		AudioRx(const char* sdp, int max_delay, put_audio_samples_rx callback);
		~AudioRx();
		int start();
		int stop();
	};
}

#endif /* __AUDIO_RX_H__ */
