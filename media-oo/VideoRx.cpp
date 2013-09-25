
#include "VideoRx.h"

extern "C" {
#include "libswscale/swscale.h"
}

static int SWS_FLAGS = SWS_BICUBIC;

using namespace media;

VideoRx::VideoRx(MediaPort* mediaPort, const char* sdp, int max_delay,
						FrameManager *frame_manager)
: MediaRx(mediaPort, sdp, max_delay, CODEC_TYPE_VIDEO)
{
	_frame_manager = frame_manager;
	_pFrame = avcodec_alloc_frame();
	LOG_TAG = "media-video-rx";
}

VideoRx::~VideoRx()
{
	av_free(_pFrame);
	_frame_manager->release_decoded_frame();
}

void
VideoRx::processPacket(AVPacket avpkt, int64_t rx_time)
{
	int len, got_picture;
	DecodedFrame *decoded_frame;
	struct SwsContext *img_convert_ctx;

	//Is this a avpkt from the video stream?
	if (avpkt.stream_index != _stream)
		return;

	while (avpkt.size > 0) {
		len = avcodec_decode_video2(_pDecodecCtx, _pFrame, &got_picture, &avpkt);
		if (len < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error in video decoding");
			break;
		}

		//Did we get a video frame?
		if (got_picture) {
			decoded_frame = _frame_manager->get_decoded_frame(
						_pDecodecCtx->width, _pDecodecCtx->height);
			if (!decoded_frame)
				break;

			//Convert the image from its native format to RGB
			img_convert_ctx = sws_getContext(_pDecodecCtx->width,
					_pDecodecCtx->height, _pDecodecCtx->pix_fmt,
					_pDecodecCtx->width, _pDecodecCtx->height, _frame_manager->pix_fmt,
					SWS_FLAGS, NULL, NULL, NULL);
			if (img_convert_ctx == NULL)
				break;
			sws_scale(img_convert_ctx, (const uint8_t* const*)_pFrame->data, _pFrame->linesize, 0,
					_pDecodecCtx->height, ((AVPicture*) decoded_frame->pFrameRGB)->data,
					((AVPicture*) decoded_frame->pFrameRGB)->linesize);
			sws_freeContext(img_convert_ctx);

			decoded_frame->width = _pDecodecCtx->width;
			decoded_frame->height = _pDecodecCtx->height;
			decoded_frame->time_base = _pFormatCtx->streams[_stream]->time_base;
			decoded_frame->pts = avpkt.pts;
			decoded_frame->start_time = _pFormatCtx->streams[_stream]->start_time;
			decoded_frame->rx_time = rx_time;
			decoded_frame->encoded_size = len;

			_frame_manager->put_video_frame_rx(decoded_frame);
		}

		avpkt.size -= len;
		avpkt.data += len;
	}
}
