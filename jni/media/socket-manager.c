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
#include <init-media.h>

#include <jni.h>
#include <pthread.h>
#include <semaphore.h>
#include <android/log.h>

#include "libavformat/rtsp.h"

enum {
	AUDIO, VIDEO,
};

static char buf[256]; //Log
static char* LOG_TAG = "NDK-socket-manager";

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
get_connection(int media_type, int port) {
	URLContext *urlContext = NULL;
	AVFormatContext *s = NULL;

	static char rtp[256];

	if (init_media() != 0) {
		__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Couldn't init media");
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

	if (media_type == AUDIO)
		s = pAudioFormatCtx;
	else if (media_type == VIDEO)
		s = pVideoFormatCtx;

	if (!s) {
		if (port != -1) {
			snprintf(rtp, sizeof(rtp), "rtp://0.0.0.0:0?localport=%d", port);
			__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, rtp);
		} else {
			snprintf(rtp, sizeof(rtp), "rtp://0.0.0.0:0");
			__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, rtp);
		}
		s = avformat_alloc_context();
		if (!s) {
			__android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
					"Memory error: Could not alloc context");
			s = NULL;
		} else if (avio_open(&s->pb, rtp, AVIO_RDWR) < 0) {
			__android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
					"Could not open 'rtp://0.0.0.0:0'");
			av_free(s);
			s = NULL;
		}
	}

	if (s && s->pb) {
		urlContext = s->pb->opaque;
		if (media_type == AUDIO) {
			pAudioFormatCtx = s;
			nAudio++;
		} else if (media_type == VIDEO) {
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

	snprintf(buf, sizeof(buf), "before nAudio: %d", nAudio);
	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, buf);
	snprintf(buf, sizeof(buf), "before nVideo: %d", nVideo);
	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, buf);

	if (pAudioFormatCtx && pAudioFormatCtx->pb && (urlContext
			== pAudioFormatCtx->pb->opaque) && (--nAudio == 0)) {
		__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "free pAudioFormatCtx");
		avio_close(pAudioFormatCtx->pb);
		avformat_free_context(pAudioFormatCtx);
		__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "free ok");
		pAudioFormatCtx = NULL;
	}
	if (pVideoFormatCtx && pVideoFormatCtx->pb && (urlContext
			== pVideoFormatCtx->pb->opaque) && (--nVideo == 0)) {
		__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "free pVideoFormatCtx");
		avio_close(pVideoFormatCtx->pb);
		avformat_free_context(pVideoFormatCtx);
		__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "free ok");
		pVideoFormatCtx = NULL;
	}
	urlContext = NULL;

	snprintf(buf, sizeof(buf), "after nAudio: %d", nAudio);
	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, buf);
	snprintf(buf, sizeof(buf), "after nVideo: %d", nVideo);
	__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, buf);

	last_released = (nAudio == 0) && (nVideo == 0);
	if (last_released) {
		snprintf(buf, sizeof(buf), "deblocked: %d", n_blocked);
		__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
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
		__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "free output");
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
			__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "free input");
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
	return get_connection(AUDIO, audioPort);
}

jint Java_com_kurento_kas_media_ports_MediaPortManager_takeAudioLocalPort(
		JNIEnv* env, jobject thiz, int audioPort) {
	snprintf(buf, sizeof(buf), "takeAudioLocalPort Port: %d", audioPort);
	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, buf);
	URLContext *urlContext = get_audio_connection(audioPort);
	return rtp_get_local_rtp_port(urlContext);
}

jint Java_com_kurento_kas_media_ports_MediaPortManager_releaseAudioLocalPort(
		JNIEnv* env, jobject thiz) {
	int ret;

	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "releaseAudioLocalPort");
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
	return get_connection(VIDEO, videoPort);
}

jint Java_com_kurento_kas_media_ports_MediaPortManager_takeVideoLocalPort(
		JNIEnv* env, jobject thiz, int videoPort) {
	snprintf(buf, sizeof(buf), "takeVideoLocalPort Port: %d", videoPort);
	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, buf);
	URLContext *urlContext = get_video_connection(videoPort);
	return rtp_get_local_rtp_port(urlContext);
}

jint Java_com_kurento_kas_media_ports_MediaPortManager_releaseVideoLocalPort(
		JNIEnv* env, jobject thiz) {
	int ret;

	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "releaseVideoLocalPort");
	if (pVideoFormatCtx && pVideoFormatCtx->pb)
		free_connection(pVideoFormatCtx->pb->opaque);

	pthread_mutex_lock(&mutex);
	ret = nVideo;
	pthread_mutex_unlock(&mutex);

	return ret;
}

