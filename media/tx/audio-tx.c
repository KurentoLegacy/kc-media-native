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
 * @author Miguel París Díaz
 * 
 */

#include "audio-tx.h"
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

static char* LOG_TAG = "media-audio-tx";

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//static int sws_flags = SWS_BICUBIC;

//Coupled with Java
int AUDIO_CODECS[] = {CODEC_ID_AMR_NB, CODEC_ID_MP2, CODEC_ID_AAC, CODEC_ID_PCM_MULAW, CODEC_ID_PCM_ALAW};
char* AUDIO_CODEC_NAMES[] = {"amr_nb", "mp2", "aac", "pcm_mulaw", "pcm_alaw"};

static uint8_t *audio_outbuf;
static int audio_outbuf_size;

static AVOutputFormat *fmt;
static AVFormatContext *oc;
static AVStream *audio_st;

static int frame_size;
enum {
	DEFAULT_FRAME_SIZE = 20, //milliseconds
};


////////////////////////////////////////////////////////////////////////////////////////
//INIT AUDIO

static int open_audio(AVFormatContext *oc, AVStream *st)
{
	AVCodec *codec;
	AVCodecContext *c;
	int ret;
	
	c = st->codec;
	
	
	/* find the audio encoder */
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
	
	audio_outbuf_size = 100000;
	audio_outbuf = av_malloc(audio_outbuf_size);
	
	return 0;
}


/*
 * add an audio output stream
 */
static AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id, int sample_rate, int bit_rate)
{
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 1);
	if (!st) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not alloc stream");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_AUDIO;

	/* put sample parameters */
	c->bit_rate = bit_rate;//AMR:12200;//MP2:64000
	c->sample_rate = sample_rate;//AMR:8000;//MP2:44100;
	c->channels = 1;
	c->sample_fmt = SAMPLE_FMT_S16;//AV_SAMPLE_FMT_S16;

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "bit_rate: %d", c->bit_rate);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sample_rate: %d", c->sample_rate);

	// some formats want stream headers to be separate
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

int
init_audio_tx(const char* outfile, int codec_id,
			 int sample_rate, int bit_rate, int payload_type) {
	int ret;
	URLContext *urlContext;

	pthread_mutex_lock(&mutex);

	if ( (ret = init_media()) != 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't init media");
		goto end;
	}
/*	
	for(i=0; i<AVMEDIA_TYPE_NB; i++){
		avcodec_opts[i]= avcodec_alloc_context2(i);
	}
	avformat_opts = avformat_alloc_context();
	sws_opts = sws_getContext(16,16,0, 16,16,0, sws_flags, NULL,NULL,NULL);
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
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Format established: %s", fmt->name);

	//FIXME check 0 <= codec_id <= size(AUDIO_CODECS)
	fmt->audio_codec = AUDIO_CODECS[codec_id];
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "audio codec_id: %d", codec_id);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Audio Codec stablished: %s", AUDIO_CODEC_NAMES[codec_id]);

	/* allocate the output media context */
	oc = avformat_alloc_context();
	if (!oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Memory error: Could not alloc context");
		ret = -2;
		goto end;
	}
	oc->oformat = fmt;
	snprintf(oc->filename, sizeof(oc->filename), "%s", outfile);

	/* add the audio stream using the default format codecs
	and initialize the codecs */
	audio_st = NULL;

	if (fmt->audio_codec != CODEC_ID_NONE) {
		audio_st = add_audio_stream(oc, fmt->audio_codec, sample_rate, bit_rate);
	}

	/* set the output parameters (must be done even if no
	parameters). */
	if (av_set_parameters(oc, NULL) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Invalid output format parameters");
		ret = -3;
		goto end;
	}

	av_dump_format(oc, 0, outfile, 1);

	/* now that all the parameters are set, we can open the
	audio codec and allocate the necessary encode buffers */
	if (audio_st) {
		if((ret = open_audio(oc, audio_st)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open audio");
			goto end;
		}
	}

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&oc->pb, outfile, URL_WRONLY)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG,
				  "Could not open '%s' AVERROR_NOENT:%d", outfile, AVERROR_NOENT);
			goto end;
		}
	}

	//Free old URLContext
	if( (ret=ffurl_close(oc->pb->opaque)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not free URLContext");
		goto end;
	}

	urlContext = get_audio_connection(0);
	if ((ret = rtp_set_remote_url (urlContext, outfile)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open '%s'", outfile);
		goto end;
	}

	oc->pb->opaque = urlContext;

	/* write the stream header, if any */
	av_write_header(oc);

	RTPMuxContext *rptmc = oc->priv_data;
	rptmc->payload_type = payload_type;
	rptmc->max_frames_per_packet = 1;

	media_log(MEDIA_LOG_INFO, LOG_TAG, "Frames per packet: %d", rptmc->max_frames_per_packet);

	if(audio_st->codec->frame_size > 1)
		frame_size = audio_st->codec->frame_size;
	else
		frame_size = sample_rate * DEFAULT_FRAME_SIZE / 1000;
	ret = frame_size;
	media_log(MEDIA_LOG_INFO, LOG_TAG, "Audio frame size: %d", frame_size);

end:
	pthread_mutex_unlock(&mutex);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//PUT AUIDO SAMPLES

static int write_audio_frame(AVFormatContext *oc, AVStream *st,
						int16_t *samples, int64_t time)
{
	AVCodecContext *c;
	AVPacket pkt;
	av_init_packet(&pkt);
	int ret;

	c = st->codec;

	pkt.size= avcodec_encode_audio(c, audio_outbuf, frame_size, samples);
	
	pkt.pts= get_pts(time, st->time_base);
	pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index= st->index;
	pkt.data= audio_outbuf;

	/* write the compressed frame in the media file */
	ret = av_write_frame(oc, &pkt);
	if (ret != 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error while writing audio frame");
		return ret;
	}

	return 0;
}

int
put_audio_samples_tx(int16_t* samples, int n_samples, int64_t time) {
	int i, ret, nframes;

	pthread_mutex_lock(&mutex);
	if (!oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "No audio initiated.");
		ret = -1;
		goto end;
	}

	nframes = n_samples / frame_size;
	for (i=0; i<nframes; i++) {
		if( (ret=write_audio_frame(oc, audio_st, &samples[i*frame_size], time)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not write audio frame");
			goto end;
		}
	}
	ret = 0;

end:
	pthread_mutex_unlock(&mutex);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//FINISH AUDIO

static void close_audio(AVFormatContext *oc, AVStream *st)
{
	if(st)
		avcodec_close(st->codec);
	av_free(audio_outbuf);
}

int
finish_audio_tx() {
	int i;
	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
	pthread_mutex_lock(&mutex);
	if(oc) {
		av_write_trailer(oc);
		/* close codec */
		if (audio_st)
			close_audio(oc, audio_st);
		/* free the streams */
		for(i = 0; i < oc->nb_streams; i++) {
			av_freep(&oc->streams[i]->codec);
			av_freep(&oc->streams[i]);
		}
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Close the context...");
		close_context(oc);
		oc = NULL;
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "ok");
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
