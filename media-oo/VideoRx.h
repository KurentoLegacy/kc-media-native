/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef __VIDEO_RX_H__
#define __VIDEO_RX_H__

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
		AVFrame *_pFrame;

	public:
		VideoRx(MediaPort* mediaPort, const char* sdp, int max_delay,
						FrameManager *frame_manager);
		~VideoRx();

	private:
		void processPacket(AVPacket avpkt, int64_t rx_time);
	};
}

#endif /* __VIDEO_RX_H__ */
