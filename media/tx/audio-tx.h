#ifndef __AUDIO_TX_H__
#define __AUDIO_TX_H__

#include <stdint.h>

int init_audio_tx(const char* outfile, int codec_id,
	      int sample_rate, int bit_rate, int payload_type);
int put_audio_samples_tx(int16_t* samples, int n_samples);
int finish_audio_tx();

#endif /* __AUDIO_TX_H__ */