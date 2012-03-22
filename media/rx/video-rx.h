#ifndef __VIDEO_RX_H__
#define __VIDEO_RX_H__

#include "libavformat/avformat.h"

typedef struct DecodedFrame {
	AVFrame* pFrameRGB;
	uint8_t *buffer;
} DecodedFrame;

typedef void (*put_video_frame_rx)(uint8_t*, int, int, int);
typedef DecodedFrame* (*get_decoded_frame_buffer)(int width, int height);
typedef void (*release_decoded_frame_buffer)();

int start_video_rx(const char* sdp, int maxDelay, put_video_frame_rx callback,
				get_decoded_frame_buffer get_buffer,
				release_decoded_frame_buffer release_buffer);
int stop_video_rx();

#endif /* __VIDEO_RX_H__ */
