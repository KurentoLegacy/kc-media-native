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

#include "audio-rx.h"
#include <util/log.h>
#include <init-media.h>
#include <socket-manager.h>

#include <pthread.h>

#include "libavformat/avformat.h"

#include "sdp-manager.h"

static char buf[256];
static char* LOG_TAG = "media-audio-rx";

enum {
	DATA_SIZE = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2,
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int receive = 0;

int
stop_audio_rx() {
	pthread_mutex_lock(&mutex);
	receive = 0;
	set_interrrupt_cb(1);
	pthread_mutex_unlock(&mutex);
	return 0;
}

int
start_audio_rx(const char* sdp, int maxDelay, put_audio_samples_rx callback) {
	AVFormatContext *pFormatCtx = NULL;
	AVFormatParameters params, *ap = &params;
	AVCodecContext *pDecodecCtxAudio = NULL;
	AVCodec *pDecodecAudio = NULL;

	AVPacket avpkt;
	uint8_t *avpkt_data_init;
	uint8_t outbuf[DATA_SIZE];
	DecodedAudioSamples decoded_samples, *ds = &decoded_samples;

	int i, ret, audioStream, out_size, len;
	int64_t rx_time;

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sdp: %s", sdp);

	if ((ret = init_media()) != 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't init media");
		goto end;
	}
	set_interrrupt_cb(0);
	
	pthread_mutex_lock(&mutex);
	receive = 1;
	pthread_mutex_unlock(&mutex);
	for(;;) {
		pthread_mutex_lock(&mutex);
		if (!receive) {
			pthread_mutex_unlock(&mutex);
			goto end;
		}
		pthread_mutex_unlock(&mutex);

		pFormatCtx = avformat_alloc_context();
		pFormatCtx->max_delay = maxDelay * 1000;
		ap->prealloced_context = 1;

		// Open audio file
		if ( (ret = av_open_input_sdp(&pFormatCtx, sdp, ap)) != 0 ) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't process sdp: %s", buf);
			ret = -2;
			goto end;
		}

		// Retrieve stream information
		if ( (ret = av_find_stream_info(pFormatCtx)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_WARN, LOG_TAG, "Couldn't find stream information: %s", buf);
			close_context(pFormatCtx);
		} else
			break;
	}

	// Find the first audio stream
	audioStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			audioStream = i;
			break;
		}
	}
	if (audioStream == -1) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Didn't find a audio stream");
		ret = -4;
		goto end;
	}

	// Get a pointer to the codec context for the audio stream
	pDecodecCtxAudio = pFormatCtx->streams[audioStream]->codec;

media_log(MEDIA_LOG_DEBUG, LOG_TAG, "time_base.num: %d", pFormatCtx->streams[audioStream]->time_base.num);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "time_base.den: %d", pFormatCtx->streams[audioStream]->time_base.den);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "start_time: %lld", pFormatCtx->streams[audioStream]->start_time);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "duration: %lld", pFormatCtx->streams[audioStream]->duration);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "r_frame_rate.num: %d", pFormatCtx->streams[audioStream]->r_frame_rate.num);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "r_frame_rate.den: %d", pFormatCtx->streams[audioStream]->r_frame_rate.den);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "avg_frame_rate.num: %d", pFormatCtx->streams[audioStream]->avg_frame_rate.num);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "avg_frame_rate.den: %d", pFormatCtx->streams[audioStream]->avg_frame_rate.den);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sample_aspect_ratio.den: %d", pFormatCtx->streams[audioStream]->sample_aspect_ratio.den);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sample_aspect_ratio.den: %d", pFormatCtx->streams[audioStream]->sample_aspect_ratio.den);

	// Find the decoder for the video stream
	if(pDecodecCtxAudio->codec_id == CODEC_ID_AMR_NB) {
		pDecodecAudio = avcodec_find_decoder_by_name("libopencore_amrnb");
	}
	else {
		pDecodecAudio = avcodec_find_decoder(pDecodecCtxAudio->codec_id);
	}

	if (pDecodecAudio == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Unsupported audio codec");
		ret = -5;
		goto end;
	}

	media_log(MEDIA_LOG_INFO, LOG_TAG, "max_delay: %d ms", pFormatCtx->max_delay/1000);
	media_log(MEDIA_LOG_INFO, LOG_TAG, "audio codec: %s", pDecodecAudio->name);

	// Open audio codec
	if (avcodec_open(pDecodecCtxAudio, pDecodecAudio) < 0) {
		ret = -6; // Could not open codec
		goto end;
	}

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Sample Rate: %d; Frame Size: %d; Bit Rate: %d",
		pDecodecCtxAudio->sample_rate, pDecodecCtxAudio->frame_size,
		pDecodecCtxAudio->bit_rate);

i = 0;
int n_packet = 0;

	//READING THE DATA
	for(;;) {
		pthread_mutex_lock(&mutex);
		if (!receive) {
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);

		if (av_read_frame(pFormatCtx, &avpkt) >= 0) {
			rx_time = av_gettime() / 1000;
			avpkt_data_init = avpkt.data;
			//Is this a avpkt from the audio stream?
			if (avpkt.stream_index == audioStream) {
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "%d -------------------", n_packet++);
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "avpkt->pts: %lld", avpkt.pts);
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "avpkt->dts: %lld", avpkt.dts);
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "avpkt->size: %d", avpkt.size);
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "dts/size: %lld", avpkt.dts / avpkt.size);
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "time: %lld s", avpkt.dts / pDecodecCtxAudio->sample_rate);
				media_log(MEDIA_LOG_DEBUG, LOG_TAG, "rx_time: %lld", rx_time);

				while (avpkt.size > 0) {
					//Decode audio frame
					//FIXME: do not reuse outbuf.
					out_size = DATA_SIZE;
					len = avcodec_decode_audio3(pDecodecCtxAudio, (int16_t*)outbuf, &out_size, &avpkt);
					if (len < 0) {
						media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error in audio decoding.");
						break;
					}
					
					pthread_mutex_lock(&mutex);
					if (!receive) {
						pthread_mutex_unlock(&mutex);
						break;
					}
					if (out_size > 0) {
						ds->samples = outbuf;
						ds->size = out_size;
						ds->time_base = pFormatCtx->streams[audioStream]->time_base;
						ds->pts = avpkt.pts;
						ds->start_time = pFormatCtx->streams[audioStream]->start_time;
						ds->rx_time = rx_time;
						callback(ds);
					}
					pthread_mutex_unlock(&mutex);
					
					avpkt.size -= len;
					avpkt.data += len;
					i++;
				}
			}
			//Free the packet that was allocated by av_read_frame
			avpkt.data = avpkt_data_init;
			av_free_packet(&avpkt);
		}
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "next");
	}

	ret = 0;

end:
	set_interrrupt_cb(0);

	//Close the codec
	if (pDecodecCtxAudio)
		avcodec_close(pDecodecCtxAudio);
	
	//Close the audio file
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Close the context...");
	close_context(pFormatCtx);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "ok");

	return ret;
}
