
#ifndef __VIDEO_RX_H__
#define __VIDEO_RX_H__

extern "C" {
#include "libavformat/avformat.h"
}

#include "MediaRx.h"

typedef struct DecodedFrame {
	AVFrame* pFrameRGB;
	uint8_t *buffer;
	int width;
	int height;
	AVRational time_base;
	uint64_t pts;
	int64_t start_time;
	int64_t rx_time;
	int encoded_size;
	void *priv_data; /* User private data */
} DecodedFrame;

typedef struct FrameManager {
	enum PixelFormat pix_fmt;
	void (*put_video_frame_rx)(DecodedFrame* decoded_frame);
	DecodedFrame* (*get_decoded_frame)(int width, int height);
	void (*release_decoded_frame)(void);
} FrameManager;

namespace media {
	class VideoRx : public MediaRx {
	private:
		FrameManager *_frame_manager;

	public:
		VideoRx(const char* sdp, int max_delay, FrameManager *frame_manager);
		~VideoRx();
		int start();
		int stop();
	};
}

#endif /* __VIDEO_RX_H__ */
