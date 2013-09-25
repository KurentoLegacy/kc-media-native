
#ifndef __VIDEO_TX_H__
#define __VIDEO_TX_H__

extern "C" {
#include "libavformat/avformat.h"
}

#include "Media.h"
#include "util/Lock.h"
#include "MediaPort.h"

namespace media {
	class VideoTx : public Media {
	private:
		MediaPort* _mediaPort;

		AVOutputFormat *_fmt;
		AVFormatContext *_oc;
		AVStream *_video_st;

		AVFrame *_picture, *_tmp_picture;
		uint8_t *_picture_buf;
		uint8_t *_video_outbuf;
		int _video_outbuf_size, _n_frame;
		enum PixelFormat _src_pix_fmt;

		Lock *_mutex;

	public:
		VideoTx(const char* outfile, int width, int height,
			int frame_rate_num, int frame_rate_den,
			int bit_rate, int gop_size, enum CodecID codec_id,
			int payload_type, enum PixelFormat src_pix_fmt,
			MediaPort* mediaPort) throw(MediaException);
		~VideoTx();
		int putVideoFrameTx(uint8_t* frame, int width, int height,
						int64_t time) throw(MediaException);
	private:
		AVStream* addVideoStream(enum CodecID codec_id,
					int width, int height, int frame_rate_num,
					int frame_rate_den, int bit_rate, int gop_size);
		void openVideo() throw(MediaException);
		void release();
	};
}

#endif /* __VIDEO_TX_H__ */
