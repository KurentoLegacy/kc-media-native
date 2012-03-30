#ifndef __AUDIO_RX_H__
#define __AUDIO_RX_H__

#include <stdint.h>
#include "libavformat/avformat.h"

typedef struct DecodedAudioSamples {
	uint8_t *samples;
	int size;
	AVRational time_base;
	uint64_t pts;
	int64_t start_time;
	void *priv_data; /* User private data */
} DecodedAudioSamples;

typedef void (*put_audio_samples_rx)(DecodedAudioSamples* decoded_audio_samples);

int start_audio_rx(const char* sdp, int maxDelay, put_audio_samples_rx callback);
int stop_audio_rx();

#endif /* __AUDIO_RX_H__ */
