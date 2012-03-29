#ifndef __AUDIO_RX_H__
#define __AUDIO_RX_H__

#include <stdint.h>

typedef void (*put_audio_samples_rx)(uint8_t *samples, int size, int nframe);

int start_audio_rx(const char* sdp, int maxDelay, put_audio_samples_rx callback);
int stop_audio_rx();

#endif /* __AUDIO_RX_H__ */
