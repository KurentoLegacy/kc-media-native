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

#include "socket-manager.h"
#include <util/log.h>
#include <init-media.h>

#include <pthread.h>
#include <semaphore.h>

#include "libavformat/rtsp.h"

static char* LOG_TAG = "media-socket-manager";
static char buf[256];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t sem;
static int initiated;
static int last_released = 1;
static int n_blocked = 0;

static AVFormatContext *pAudioFormatCtx;
static int nAudio;

static AVFormatContext *pVideoFormatCtx;
static int nVideo;

static URLContext*
get_connection(enum AVMediaType media_type, int port) {
	int ret;

	URLContext *urlContext = NULL;
	AVFormatContext *s = NULL;

	static char rtp[256];

	if (init_media() != 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't init media");
		return NULL;
	}

	pthread_mutex_lock(&mutex);

	if (!initiated) {
		sem_init(&sem, 0, 0);
		initiated = 1;
	}
	if (!last_released) {
		n_blocked++;
		pthread_mutex_unlock(&mutex);
		sem_wait(&sem);
		pthread_mutex_lock(&mutex);
		n_blocked--;
	}

	if (media_type == AVMEDIA_TYPE_AUDIO)
		s = pAudioFormatCtx;
	else if (media_type == AVMEDIA_TYPE_VIDEO)
		s = pVideoFormatCtx;

	if (!s) {
		if (port != -1) {
			snprintf(rtp, sizeof(rtp), "rtp://0.0.0.0:0?localport=%d", port);
			media_log(MEDIA_LOG_DEBUG, LOG_TAG, rtp);
		} else {
			snprintf(rtp, sizeof(rtp), "rtp://0.0.0.0:0");
			media_log(MEDIA_LOG_DEBUG, LOG_TAG, rtp);
		}
		s = avformat_alloc_context();
		if (!s) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG,
					"Memory error: Could not alloc context");
			s = NULL;
		} else if ((ret = avio_open(&s->pb, rtp, AVIO_RDWR)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_ERROR, LOG_TAG,
					"Could not open '%s': %s", rtp, buf);
			av_free(s);
			s = NULL;
		}
	}

	if (s && s->pb) {
		urlContext = s->pb->opaque;
		if (media_type == AVMEDIA_TYPE_AUDIO) {
			pAudioFormatCtx = s;
			nAudio++;
		} else if (media_type == AVMEDIA_TYPE_VIDEO) {
			pVideoFormatCtx = s;
			nVideo++;
		}
	}

	pthread_mutex_unlock(&mutex);

	return urlContext;
}

static void free_connection(URLContext *urlContext) {

	int i;

	pthread_mutex_lock(&mutex);

	media_log(MEDIA_LOG_ERROR, LOG_TAG, "before nAudio: %d", nAudio);
	media_log(MEDIA_LOG_ERROR, LOG_TAG, "before nVideo: %d", nVideo);

	if (pAudioFormatCtx && pAudioFormatCtx->pb && (urlContext
			== pAudioFormatCtx->pb->opaque) && (--nAudio == 0)) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free pAudioFormatCtx");
		avio_close(pAudioFormatCtx->pb);
		avformat_free_context(pAudioFormatCtx);
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free ok");
		pAudioFormatCtx = NULL;
	}
	if (pVideoFormatCtx && pVideoFormatCtx->pb && (urlContext
			== pVideoFormatCtx->pb->opaque) && (--nVideo == 0)) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free pVideoFormatCtx");
		avio_close(pVideoFormatCtx->pb);
		avformat_free_context(pVideoFormatCtx);
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free ok");
		pVideoFormatCtx = NULL;
	}
	urlContext = NULL;

	media_log(MEDIA_LOG_ERROR, LOG_TAG, "after nAudio: %d", nAudio);
	media_log(MEDIA_LOG_ERROR, LOG_TAG, "after nVideo: %d", nVideo);

	last_released = (nAudio == 0) && (nVideo == 0);
	if (last_released) {
		media_log(MEDIA_LOG_INFO, LOG_TAG, "deblocked: %d", n_blocked);
		for (i = 0; i < n_blocked; i++)
			sem_post(&sem);
	}

	pthread_mutex_unlock(&mutex);
}

void close_context(AVFormatContext *s) {
	RTSPState *rt;
	RTSPStream *rtsp_st;
	int i;

	if (!s)
		return;

	//if is output
	if (s->oformat && s->pb) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free output");
		free_connection(s->pb->opaque);
		s->pb->opaque = NULL;
		if (!(s->oformat->flags & AVFMT_NOFILE)) {
			avio_close(s->pb);
		}
		av_free(s);
	}

	//if is input
	if (s->iformat && s->priv_data) {
		rt = s->priv_data;
		for (i = 0; i < rt->nb_rtsp_streams; i++) {
			rtsp_st = rt->rtsp_streams[i];
			media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free input");
			free_connection(rtsp_st->rtp_handle);
			rtsp_st->rtp_handle = NULL;
		}
		av_close_input_file(s);
	}
}

URLContext*
get_connection_by_local_port(int local_port) {
	URLContext *urlContext = NULL;

	pthread_mutex_lock(&mutex);

	if (pVideoFormatCtx && (rtp_get_local_rtp_port(pVideoFormatCtx->pb->opaque)
			== local_port)) {
		urlContext = pVideoFormatCtx->pb->opaque;
		nVideo++;
	} else if (pAudioFormatCtx && (rtp_get_local_rtp_port(
			pAudioFormatCtx->pb->opaque) == local_port)) {
		urlContext = pAudioFormatCtx->pb->opaque;
		nAudio++;
	}

	pthread_mutex_unlock(&mutex);

	return urlContext;
}

//AUDIO

URLContext*
get_audio_connection(int audioPort) {
	return get_connection(AVMEDIA_TYPE_AUDIO, audioPort);
}

int
take_audio_local_port(int audioPort) {
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "takeAudioLocalPort Port: %d", audioPort);
	URLContext *urlContext = get_audio_connection(audioPort);
	if (!urlContext)
		return -1;
	return rtp_get_local_rtp_port(urlContext);
}

int
release_audio_local_port() {
	int ret;

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "releaseAudioLocalPort");
	if (pAudioFormatCtx && pAudioFormatCtx->pb)
		free_connection(pAudioFormatCtx->pb->opaque);

	pthread_mutex_lock(&mutex);
	ret = nAudio;
	pthread_mutex_unlock(&mutex);

	return ret;
}

//VIDEO

URLContext*
get_video_connection(int videoPort) {
	return get_connection(AVMEDIA_TYPE_VIDEO, videoPort);
}

int
take_video_local_port(int videoPort) {
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "takeVideoLocalPort Port: %d", videoPort);
	URLContext *urlContext = get_video_connection(videoPort);
	if (!urlContext)
		return -1;
	return rtp_get_local_rtp_port(urlContext);
}

int
release_video_local_port() {
	int ret;

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "releaseVideoLocalPort");
	if (pVideoFormatCtx && pVideoFormatCtx->pb)
		free_connection(pVideoFormatCtx->pb->opaque);

	pthread_mutex_lock(&mutex);
	ret = nVideo;
	pthread_mutex_unlock(&mutex);

	return ret;
}
