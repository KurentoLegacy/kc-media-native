#ifndef __VIDEO_TX_H__
#define __VIDEO_TX_H__

#include <stdint.h>
#include <libavutil/pixfmt.h>

int init_video_tx(const char* outfile, int width, int height,
		  int frame_rate_num, int frame_rate_den,
		  int bit_rate, int gop_size, int codecId,
		  int payload_type, enum PixelFormat src_pix_fmt);
int put_video_frame_tx(uint8_t* frame, int width, int height, int64_t time);
int finish_video_tx();

#endif /* __VIDEO_TX_H__ */
