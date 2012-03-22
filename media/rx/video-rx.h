#ifndef __VIDEO_RX_H__
#define __VIDEO_RX_H__

#include "libavformat/avformat.h"

typedef struct DecodedFrame {
	AVFrame* pFrameRGB;
	uint8_t *buffer;
} DecodedFrame;

typedef struct FrameManager {
	enum PixelFormat pix_fmt;
	void (*put_video_frame_rx)(uint8_t *data, int with, int height, int nframe);
	DecodedFrame* (*get_decoded_frame_buffer)(int width, int height);
	void (*release_decoded_frame_buffer)(void);
} FrameManager;

int start_video_rx(const char* sdp, int maxDelay, FrameManager *frame_manager);
int stop_video_rx();

#endif /* __VIDEO_RX_H__ */
