
#include "VideoTx.h"
#include "MediaPortManager.h"

extern "C" {
#include "util/utils.h"

#include "libswscale/swscale.h"
#include "libavcodec/opt.h"
#include "libavformat/rtpenc.h"
#include "libavformat/rtpdec.h"
#include "libavformat/url.h"
}

static int SWS_FLAGS = SWS_BICUBIC;

using namespace media;
//TODO: methods as synchronized
VideoTx::VideoTx(const char* outfile, int width, int height,
			int frame_rate_num, int frame_rate_den,
			int bit_rate, int gop_size, enum CodecID codec_id,
			int payload_type, enum PixelFormat src_pix_fmt,
			MediaPort* mediaPort)
: Media()
{
	int ret;
	URLContext *urlContext;
	RTPMuxContext *rptmc;

	LOG_TAG = "media-video-tx";
	_mediaPort = mediaPort;

#ifndef USE_X264
	media_log(MEDIA_LOG_INFO, LOG_TAG, "USE_X264 no def");
	/* TODO: Improve this hack to disable H264 */
	if (codec_id == CODEC_ID_H264) {
		media_log(MEDIA_LOG_WARN, LOG_TAG, "H264 not supported");
		ret = -1;
		goto end;
	}
#endif

	_src_pix_fmt = src_pix_fmt;

	_fmt = av_guess_format(NULL, outfile, NULL);
	if (!_fmt) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Could not deduce output "
					"format from file extension: using RTP.");
		_fmt = av_guess_format("rtp", NULL, NULL);
	}
	if (!_fmt) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not find suitable "
								"output format");
		ret = -1;
		goto end;
	}
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Format established: %s", _fmt->name);
	_fmt->video_codec = codec_id;

	/* allocate the output media context */
	_oc = avformat_alloc_context();
	if (!_oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Memory error");
		ret = -2;
		goto end;
	}
	_oc->oformat = _fmt;
	snprintf(_oc->filename, sizeof(_oc->filename), "%s", outfile);

	/* add the  video stream using the default format codecs
	and initialize the codecs */
	_video_st = NULL;

	if (_fmt->video_codec != CODEC_ID_NONE) {
		_video_st = VideoTx::addVideoStream(_fmt->video_codec, width, height,
				frame_rate_num, frame_rate_den, bit_rate, gop_size);
		if(!_video_st) {
			ret = -3;
			goto end;
		}
	}

	/* set the output parameters (must be done even if no
	parameters). */
	if (av_set_parameters(_oc, NULL) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Invalid output format parameters");
		ret = -4;
		goto end;
	}

	av_dump_format(_oc, 0, outfile, 1);

	/* now that all the parameters are set, we can open the
	video codec and allocate the necessary encode buffers */
	if (_video_st) {
		if((ret = VideoTx::openVideo()) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open video");
			goto end;
		}
	}

	/* open the output file, if needed */
	if (!(_fmt->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&_oc->pb, outfile, URL_WRONLY)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open '%s'", outfile);
			goto end;
		}
	}

	//Free old URLContext
	if ((ret=ffurl_close((URLContext*)_oc->pb->opaque)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not free URLContext");
		goto end;
	}

	urlContext = _mediaPort->getConnection();//(URLContext*)get_video_connection(0);
	if ((ret=rtp_set_remote_url (urlContext, outfile)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG,
			  "Could not open '%s' AVERROR_NOENT:%d", outfile, AVERROR_NOENT);
		goto end;
	}

	_oc->pb->opaque = urlContext;

	/* write the stream header, if any */
	av_write_header(_oc);

	rptmc = (RTPMuxContext*)_oc->priv_data;
	rptmc->payload_type = payload_type;

	_n_frame = 0;
	ret = 0;

end:
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Constructor ret: %d", ret);
	_mutex = new Lock();
	//return ret;
	;
}

VideoTx::~VideoTx()
{
	int i;

	_mutex->lock();
	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
	if (_oc) {
		av_write_trailer(_oc);
		/* close codec */
		if (_video_st) {
			avcodec_close(_video_st->codec);
			av_free(_picture_buf);
			av_free(_picture);
			av_free(_tmp_picture);
			av_free(_video_outbuf);
		}
		/* free the streams */
		for(i = 0; i < _oc->nb_streams; i++) {
			av_freep(&_oc->streams[i]->codec);
			av_freep(&_oc->streams[i]);
		}
		//close_context(_oc);
		_mediaPort->closeContext(_oc);
		MediaPortManager::releaseMediaPort(_mediaPort);
		_oc = NULL;
	}

	_mutex->unlock();
	delete _mutex;
}

/**
 * see ffmpeg.c
 */
int
VideoTx::putVideoFrameTx(uint8_t* frame, int width, int height, int64_t time)
{
	int out_size, ret;
	AVCodecContext *c;
	struct SwsContext *img_convert_ctx;

	_mutex->lock();
	if (!_oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "No video initiated.");
		ret = -1;
		goto end;
	}

	avpicture_fill((AVPicture *)_tmp_picture, frame,
						_src_pix_fmt, width, height);

	c = _video_st->codec;

	img_convert_ctx = sws_getContext(width, height, _src_pix_fmt,
					c->width, c->height, c->pix_fmt,
					SWS_FLAGS, NULL, NULL, NULL);
	if (img_convert_ctx == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Cannot initialize the conversion context");
		ret = -1;
		goto end;
	}

	sws_scale(img_convert_ctx, (const uint8_t* const*)_tmp_picture->data, _tmp_picture->linesize,
		0, c->height, _picture->data, _picture->linesize);

	sws_freeContext(img_convert_ctx);

	if (_oc->oformat->flags & AVFMT_RAWPICTURE) {
		/* raw video case. The API will change slightly in the near
		futur for that */
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index= _video_st->index;
		pkt.data= (uint8_t *)_picture;
		pkt.size= sizeof(AVPicture);
		ret = av_interleaved_write_frame(_oc, &pkt);
	} else {
		_picture->pts = _n_frame++;
		/* encode the image */
		out_size = avcodec_encode_video(c, _video_outbuf, _video_outbuf_size, _picture);
		/* if zero size, it means the image was buffered */
		if (out_size > 0) {
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.pts = get_pts(time, _video_st->time_base);
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index= _video_st->index;
			pkt.data= _video_outbuf;
			pkt.size= out_size;

			/* write the compressed frame in the media file */
			ret = av_write_frame(_oc, &pkt);
		} else {
			ret = 0;
		}
	}

	if (ret < 0)
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error while writing video frame");
	else
		ret = out_size;

	if (ret < 0)
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not write video frame");

end:
	_mutex->unlock();
	return ret;
}


/**
 * add a video output stream
 * see new_video_stream in ffmpeg.c
 */
AVStream*
VideoTx::addVideoStream(enum CodecID codec_id, int width, int height,
		int frame_rate_num, int frame_rate_den, int bit_rate, int gop_size)
{
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(_oc, 0);
	if (!st) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not alloc stream");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_VIDEO;

	/* put sample parameters */
	c->bit_rate = bit_rate;
	//c->rc_max_rate = bit_rate;
	//c->rc_min_rate = bit_rate;
	c->qcompress = 0.0;
	c->qblur = 0.0;

	/* resolution must be a multiple of two */
	c->width = width;
	c->height = height;
	/* time base: this is the fundamental unit of time (in seconds) in terms
	of which frame timestamps are represented. for fixed-fps content,
	timebase should be 1/framerate and timestamp increments should be
	identically 1. */
	c->time_base.den = frame_rate_num;
	c->time_base.num = frame_rate_den;
	c->gop_size = gop_size;
	c->keyint_min = gop_size;
	c->max_b_frames = 0;
	//c->qmax = qmax;
	c->pix_fmt = PIX_FMT_YUV420P;

	//c->rc_buffer_size = INT_MAX;	//((c->bit_rate * (int64_t)c->time_base.num) / (int64_t)c->time_base.den) + 1;

	if (c->codec_id == CODEC_ID_MPEG2VIDEO) {
		/* just for testing, we also add B frames */
		c->max_b_frames = 2;
	}
	if (c->codec_id == CODEC_ID_MPEG1VIDEO){
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		This does not happen with normal video, it just happens here as
		the motion of the chroma plane does not match the luma plane. */
		c->mb_decision=2;
	}
	if(c->codec_id == CODEC_ID_H264) {
		c->coder_type = 0; // coder=0
		c->weighted_p_pred = 0; // wpredp=0
		c->aq_mode=0; // aq_mode=0
		c->b_frame_strategy = 0; // b_strategy=0
		c->me_method = ME_EPZS; // me_method=dia
		c->me_range = 16; // me_range=16
		c->partitions &= ~X264_PART_I4X4 & ~X264_PART_I8X8 & ~X264_PART_P8X8 & ~X264_PART_P4X4 & ~X264_PART_B8X8; // partitions=-parti8x8-parti4x4-partp8x8-partp4x4-partb8x8
		c->rc_lookahead = 0; // rc_lookahead=0
		c->refs = 1; // refs=1
		c->scenechange_threshold = 0; // sc_threshold=0
		c->me_subpel_quality = 0; // subq=0
		c->directpred = 1; // directpred=1
		c->trellis = 0; // trellis=0
		c->flags &= ~CODEC_FLAG_INTERLACED_DCT & ~CODEC_FLAG_LOOP_FILTER; // flags=-ildct-loop
		c->flags2 &= ~CODEC_FLAG2_8X8DCT & ~CODEC_FLAG2_MBTREE & ~CODEC_FLAG2_MIXED_REFS & ~CODEC_FLAG2_WPRED; // flags2=-dct8x8-mbtree-mixed_refs-wpred
		//c->qcompress = 0.6;

		//Ensure no default settings
		c->qmin = 1;
		c->qmax = 51;
		c->max_qdiff = 4;
	}

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "bit_rate: %d", c->bit_rate);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "rc_min_rate: %d", c->rc_min_rate);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "rc_max_rate: %d", c->rc_max_rate);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG,
		  "gop_size: %d\tkeyint_min: %d\tmax_b_frames: %d",
		  c->gop_size, c->keyint_min, c->max_b_frames);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "qmin: %d", c->qmin);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "qmax: %d", c->qmax);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "qcompress: %f", c->qcompress);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "qblur: %f", c->qblur);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "max_qdiff: %d", c->max_qdiff);

	// some formats want stream headers to be separate
	if(_oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}


int
VideoTx::openVideo()
{
	int ret, size;

	AVCodec *codec;
	AVCodecContext *c;

	c = _video_st->codec;

	/* find the video encoder */
	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Codec not found");
		return -1;
	}

	/* open the codec */
	if ((ret = avcodec_open(c, codec)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open codec");
		return ret;
	}

	_video_outbuf = NULL;
	if (!(_oc->oformat->flags & AVFMT_RAWPICTURE)) {
		/* allocate output buffer */
		/* XXX: API change will be done */
		/* buffers passed into lav* can be allocated any way you prefer,
		as long as they're aligned enough for the architecture, and
		they're freed appropriately (such as using av_free for buffers
		allocated with av_malloc) */
		_video_outbuf_size = 800000;
		_video_outbuf = (uint8_t*)av_malloc(_video_outbuf_size);
	}

	/* allocate the encoded raw picture */
	_picture = avcodec_alloc_frame();
	if (!_picture) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not allocate picture");
		return -3;
	}
	size = avpicture_get_size(c->pix_fmt, c->width, c->height);
	_picture_buf = (uint8_t*)av_malloc(size);
	avpicture_fill((AVPicture *)_picture, _picture_buf,
			c->pix_fmt, c->width, c->height);

	_tmp_picture =  avcodec_alloc_frame();
	if (!_tmp_picture) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not allocate temporary picture");
		return -4;
	}

	return 0;
}
