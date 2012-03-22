#ifndef __VIDEO_RX_H__
#define __VIDEO_RX_H__

#include "libavformat/avformat.h"

typedef struct DecodedFrame {
	AVFrame* pFrameRGB;
	uint8_t *buffer;
} DecodedFrame;

typedef struct FrameManager {
	void (*put_video_frame_rx)(uint8_t*, int, int, int);
	DecodedFrame* (*get_decoded_frame_buffer)(int width, int height);
	void (*release_decoded_frame_buffer)();
} FrameManager;

int start_video_rx(const char* sdp, int maxDelay, FrameManager *frame_manager);
int stop_video_rx();

#endif /* __VIDEO_RX_H__ */
