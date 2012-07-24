
#include "VideoRx.h"

extern "C" {
#include <socket-manager.h>

#include "libswscale/swscale.h"
}

static int SWS_FLAGS = SWS_BICUBIC;

using namespace media;

VideoRx::VideoRx(const char* sdp, int max_delay, FrameManager *frame_manager)
: MediaRx(sdp, max_delay)
{
	_frame_manager = frame_manager;
	LOG_TAG = "media-video-rx";
}

VideoRx::~VideoRx()
{

}

int
VideoRx::start()
{
	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pDecodecCtxVideo = NULL;
	AVCodec *pDecodecVideo = NULL;
	AVFrame *pFrame = NULL;
	DecodedFrame *decoded_frame;

	AVPacket avpkt;
	uint8_t *avpkt_data_init;

	int i, ret, videoStream, len, got_picture;
	int64_t rx_time;

	struct SwsContext *img_convert_ctx;

	_freeLock->lock();
	this->setReceive(true);
	if ((ret = MediaRx::openFormatContext(&pFormatCtx)) < 0)
		goto end;
	media_log(MEDIA_LOG_INFO, LOG_TAG, "max_delay: %d ms", pFormatCtx->max_delay/1000);

	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Didn't find a video stream");
		ret = -4;
		goto end;
	}

	// Get a pointer to the codec context for the video stream
	pDecodecCtxVideo = pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pDecodecVideo = avcodec_find_decoder(pDecodecCtxVideo->codec_id);
	if (pDecodecVideo == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Unsupported video codec");
		ret = -5; // Codec not found
		goto end;
	}

	// Open video codec
	if (avcodec_open(pDecodecCtxVideo, pDecodecVideo) < 0) {
		ret = -6; // Could not open codec
		goto end;
	}

	//STORING THE DATA
	//Allocate video frame
	pFrame = avcodec_alloc_frame();

	//READING THE DATA
	for(;;) {
		if (!this->getReceive())
			break;

		if (av_read_frame(pFormatCtx, &avpkt) >= 0) {
			rx_time = av_gettime() / 1000;
			avpkt_data_init = avpkt.data;
			//Is this a avpkt from the video stream?
			if (avpkt.stream_index == videoStream) {
				while (avpkt.size > 0) {
					len = avcodec_decode_video2(pDecodecCtxVideo, pFrame, &got_picture, &avpkt);
					if (len < 0) {
						media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error in video decoding");
						break;
					}
					if (!this->getReceive())
						break;

					//Did we get a video frame?
					if (got_picture) {
						decoded_frame = _frame_manager->get_decoded_frame(
									pDecodecCtxVideo->width, pDecodecCtxVideo->height);
						if (!decoded_frame)
							break;

						//Convert the image from its native format to RGB
						img_convert_ctx = sws_getContext(pDecodecCtxVideo->width,
								pDecodecCtxVideo->height, pDecodecCtxVideo->pix_fmt,
								pDecodecCtxVideo->width, pDecodecCtxVideo->height, _frame_manager->pix_fmt,
								SWS_FLAGS, NULL, NULL, NULL);
						if (img_convert_ctx == NULL) {
							ret = -9;
							goto end;
						}
						sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
								pDecodecCtxVideo->height, ((AVPicture*) decoded_frame->pFrameRGB)->data,
								((AVPicture*) decoded_frame->pFrameRGB)->linesize);
						sws_freeContext(img_convert_ctx);

						decoded_frame->width = pDecodecCtxVideo->width;
						decoded_frame->height = pDecodecCtxVideo->height;
						decoded_frame->time_base = pFormatCtx->streams[videoStream]->time_base;
						decoded_frame->pts = avpkt.pts;
						decoded_frame->start_time = pFormatCtx->streams[videoStream]->start_time;
						decoded_frame->rx_time = rx_time;
						decoded_frame->encoded_size = len;

						_frame_manager->put_video_frame_rx(decoded_frame);
					}

					avpkt.size -= len;
					avpkt.data += len;
				}
			}
			//Free the packet that was allocated by av_read_frame
			avpkt.data = avpkt_data_init;
			av_free_packet(&avpkt);
		}
	}

	ret = 0;

end:
	_frame_manager->release_decoded_frame();
	av_free(pFrame);

	if (pDecodecCtxVideo)
		avcodec_close(pDecodecCtxVideo);
	close_context(pFormatCtx);
	_freeLock->unlock();

	return ret;
}

int
VideoRx::stop()
{
	set_interrrupt_cb(1);
	this->setReceive(false);
	_freeLock->lock();
	_freeLock->unlock();

	return 0;
}
