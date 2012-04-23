/*
 * Kurento Android Media: Android Media Library based on FFmpeg.
 * Copyright (C) 2011  Tikal Technologies
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * 
 * @author Miguel París Díaz * 
 */

#include "video-tx.h"
#include <util/log.h>
#include <my-cmdutils.h>
#include <init-media.h>
#include <socket-manager.h>

#include <pthread.h>

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/opt.h"
#include "libavformat/rtpenc.h"
#include "libavformat/rtpdec.h"
#include "libavformat/url.h"

/*
	see	libavformat/output-example.c
		libavcodec/api-example.c
*/

static char* LOG_TAG = "media-video-tx";

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int sws_flags = SWS_BICUBIC;

enum {
	OUTBUF_SIZE = 800*1024,
};

//Coupled with Java
int VIDEO_CODECS[] = {CODEC_ID_H264, CODEC_ID_MPEG4, CODEC_ID_H263P};
char* VIDEO_CODEC_NAMES[] = {"h264", "mpeg4", "h263p"};



//FIXME Tener en cuenta si hay error, que deberemos liberar las estructuras


static AVFrame *picture, *tmp_picture;
static uint8_t *video_outbuf;
static int video_outbuf_size;

static AVOutputFormat *fmt;
static AVFormatContext *oc;
static AVStream *video_st;

////////////////////////////////////////////////////////////////////////////////////////
//INIT VIDEO

static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;

	picture = avcodec_alloc_frame();
	if (!picture)
		return NULL;
	
	return picture;
}


static int open_video(AVFormatContext *oc, AVStream *st)
{
	int ret;
	
	AVCodec *codec;
	AVCodecContext *c;
	
	c = st->codec;
	
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
	

	video_outbuf = NULL;
	if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
		/* allocate output buffer */
		/* XXX: API change will be done */
		/* buffers passed into lav* can be allocated any way you prefer,
		as long as they're aligned enough for the architecture, and
		they're freed appropriately (such as using av_free for buffers
		allocated with av_malloc) */
		video_outbuf_size = 800000;
		video_outbuf = av_malloc(video_outbuf_size);
	}

	/* allocate the encoded raw picture */
	picture = alloc_picture(c->pix_fmt, c->width, c->height);
	
	
	
	if (!picture) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not allocate picture");
		return -3;
	}

	/* if the output format is not YUV420P, then a temporary YUV420P
	picture is needed too. It is then converted to the required
	output format */
	tmp_picture =  alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
	if (!tmp_picture) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not allocate temporary picture");
		return -4;
	}
	
	return 0;
}


/**
 * add a video output stream
 * see new_video_stream in ffmpeg.c
 */
static AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id, int width, int height,
				int frame_rate_num, int frame_rate_den, int bit_rate, int gop_size)
{
	AVCodecContext *c;
	AVStream *st;
	
	st = av_new_stream(oc, 0);
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
		c->qmax = 32;
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
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

int
init_video_tx(const char* outfile, int width, int height, int frame_rate_num, int frame_rate_den,
			int bit_rate, int gop_size, int codecId, int payload_type) {
	int ret;
	URLContext *urlContext;

	pthread_mutex_lock(&mutex);
	fprintf(stderr, "fprintf test\n");

#ifndef USE_X264
	media_log(MEDIA_LOG_INFO, LOG_TAG, "USE_X264 no def");
	/* TODO: Improve this hack to disable H264 */
	if (VIDEO_CODECS[codecId] == CODEC_ID_H264) {
		media_log(MEDIA_LOG_WARN, LOG_TAG, "H264 not supported");
		ret = -1;
		goto end;
	}
#endif

	if ( (ret= init_media()) != 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't init media");
		goto end;
	}
/*	
	for(i=0; i<AVMEDIA_TYPE_NB; i++){
		avcodec_opts[i]= avcodec_alloc_context2(i);
	}
	avformat_opts = avformat_alloc_context();
	sws_opts = sws_getContext(16,16,0, 16,16,0, sws_flags, NULL,NULL,NULL);a
*/
	/* auto detect the output format from the name. default is mp4. */
	fmt = av_guess_format(NULL, outfile, NULL);
	if (!fmt) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Could not deduce output format from file extension: using RTP.");
		fmt = av_guess_format("rtp", NULL, NULL);
	}
	if (!fmt) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not find suitable output format");
		ret = -1;
		goto end;
	}
	fmt->video_codec = VIDEO_CODECS[codecId];

	/* allocate the output media context */
	oc = avformat_alloc_context();
	if (!oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Memory error");
		ret = -2;
		goto end;
	}
	oc->oformat = fmt;
	snprintf(oc->filename, sizeof(oc->filename), "%s", outfile);

	/* add the  video stream using the default format codecs
	and initialize the codecs */
	video_st = NULL;

	if (fmt->video_codec != CODEC_ID_NONE) {
		video_st = add_video_stream(oc, fmt->video_codec, width, height, frame_rate_num, frame_rate_den, bit_rate, gop_size);
		if(!video_st) {
			ret = -3;
			goto end;
		}
	}

	/* set the output parameters (must be done even if no
	parameters). */
	if (av_set_parameters(oc, NULL) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Invalid output format parameters");
		ret = -4;
		goto end;
	}
	
	av_dump_format(oc, 0, outfile, 1);

	/* now that all the parameters are set, we can open the
	video codec and allocate the necessary encode buffers */
	if (video_st) {
		if((ret = open_video(oc, video_st)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open video");
			goto end;
		}
	}

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&oc->pb, outfile, URL_WRONLY)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open '%s'", outfile);
			goto end;
		}
	}

	//Free old URLContext
	if ( (ret=ffurl_close(oc->pb->opaque)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not free URLContext");
		goto end;
	}

	urlContext = get_video_connection(0);
	if ((ret=rtp_set_remote_url (urlContext, outfile)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG,
			  "Could not open '%s' AVERROR_NOENT:%d", outfile, AVERROR_NOENT);
		goto end;
	}

	oc->pb->opaque = urlContext;

	/* write the stream header, if any */
	av_write_header(oc);
	
	RTPMuxContext *rptmc = oc->priv_data;
	rptmc->payload_type = payload_type;

	ret = 0;

end:
	pthread_mutex_unlock(&mutex);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//PUT VIDEO FRAME

/**
 * see ffmpeg.c
 */
static int write_video_frame(AVFormatContext *oc, AVStream *st,
			enum PixelFormat pix_fmt, int srcWidth, int srcHeight)
{
	int out_size, ret;
	AVCodecContext *c;
	struct SwsContext *img_convert_ctx;

	c = st->codec;
	
	img_convert_ctx = sws_getContext(srcWidth, srcHeight,
					pix_fmt,
					c->width, c->height,
					c->pix_fmt,
					sws_flags, NULL, NULL, NULL);
	if (img_convert_ctx == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Cannot initialize the conversion context");
		return -1;
	}
	sws_scale(img_convert_ctx, (const uint8_t* const*)tmp_picture->data, tmp_picture->linesize,
		0, c->height, picture->data, picture->linesize);
	sws_freeContext(img_convert_ctx);
	
	if (oc->oformat->flags & AVFMT_RAWPICTURE) {
		/* raw video case. The API will change slightly in the near
		futur for that */
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index= st->index;
		pkt.data= (uint8_t *)picture;
		pkt.size= sizeof(AVPicture);
		ret = av_interleaved_write_frame(oc, &pkt);
	} else {
		/* encode the image */
		out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
		/* if zero size, it means the image was buffered */
		if (out_size > 0) {
			AVPacket pkt;
			av_init_packet(&pkt);
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index= st->index;
			pkt.data= video_outbuf;
			pkt.size= out_size;
		
			/* write the compressed frame in the media file */
			ret = av_write_frame(oc, &pkt);
		} else {
			ret = 0;
		}
	}

	if (ret < 0)
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error while writing video frame");

	return ret;
}

int
put_video_frame_tx(enum PixelFormat pix_fmt, uint8_t* frame, int width, int height)
{
	int ret;
	uint8_t *picture2_buf;
	int size;

	pthread_mutex_lock(&mutex);

	if (!oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "No video initiated.");
		ret = -1;
		goto end;
	}
	size = avpicture_get_size(video_st->codec->pix_fmt, video_st->codec->width, video_st->codec->height);
	picture2_buf = av_malloc(size);
	avpicture_fill((AVPicture *)picture, picture2_buf,
			video_st->codec->pix_fmt, video_st->codec->width, video_st->codec->height);

	//Asociamos el frame a tmp_picture por si el pix_fmt es distinto de PIX_FMT_YUV420P
	avpicture_fill((AVPicture *)tmp_picture, frame, pix_fmt, width, height);

	if (write_video_frame(oc, video_st, pix_fmt, width, height) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not write video frame");
		ret = -2;
		goto end;
	}

	av_free(picture2_buf);
	ret = 0;

end:
	pthread_mutex_unlock(&mutex);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//FINISH VIDEO

static void close_video(AVFormatContext *oc, AVStream *st)
{
	if (st)
		avcodec_close(st->codec);
	av_free(picture);
	av_free(tmp_picture);
	av_free(video_outbuf);
}


int
finish_video_tx() {
	int i;
	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
	pthread_mutex_lock(&mutex);
	if (oc) {
		av_write_trailer(oc);		
		/* close codec */
		if (video_st)
			close_video(oc, video_st);
		/* free the streams */
		for(i = 0; i < oc->nb_streams; i++) {
			av_freep(&oc->streams[i]->codec);
			av_freep(&oc->streams[i]);
		}
		close_context(oc);
		oc = NULL;
	}	
/*	for (i=0;i<AVMEDIA_TYPE_NB;i++)
		av_free(avcodec_opts[i]);
	av_free(avformat_opts);
	av_free(sws_opts);
*/
	pthread_mutex_unlock(&mutex);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////

