/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "Media.h"

extern "C" {
#include <pthread.h>

#include "libavformat/avformat.h"
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_interrupt = PTHREAD_MUTEX_INITIALIZER;
static int initialized = 0;
static int interrupt = 0;

using namespace media;

/*
	see	libavutil/log.c
		ffserver.c
*/
static void
media_av_log(void *ptr, int level, const char *fmt, va_list vargs)
{
	MediaLogLevel media_log_level =  MEDIA_LOG_UNKNOWN;

	switch(level){
	case AV_LOG_QUIET:
		media_log_level = MEDIA_LOG_UNKNOWN;
		break;
	case AV_LOG_PANIC:
		media_log_level = MEDIA_LOG_FATAL;
		break;
	case AV_LOG_FATAL:
		media_log_level = MEDIA_LOG_FATAL;
		break;
	case AV_LOG_ERROR:
		media_log_level = MEDIA_LOG_ERROR;
		break;
	case AV_LOG_WARNING:
		media_log_level = MEDIA_LOG_WARN;
		break;
	case AV_LOG_INFO:
		media_log_level = MEDIA_LOG_INFO;
		break;
	case AV_LOG_VERBOSE:
		media_log_level = MEDIA_LOG_VERBOSE;
		break;
	case AV_LOG_DEBUG:
		media_log_level = MEDIA_LOG_DEBUG;
		break;
	}

	media_vlog(media_log_level, "av_log", fmt, vargs);
}

static int
lockmgr(void **mtx, enum AVLockOp op)
{
	switch(op) {
	case AV_LOCK_CREATE:
		*mtx = malloc(sizeof(pthread_mutex_t));
		if(!*mtx)
			return 1;
		return !!pthread_mutex_init((pthread_mutex_t*)*mtx, NULL);
	case AV_LOCK_OBTAIN:
		return !!pthread_mutex_lock((pthread_mutex_t*)*mtx);
	case AV_LOCK_RELEASE:
		return !!pthread_mutex_unlock((pthread_mutex_t*)*mtx);
	case AV_LOCK_DESTROY:
		pthread_mutex_destroy((pthread_mutex_t*)*mtx);
		free(*mtx);
		return 0;
	}
	return 1;
}

//FIXME: for newer ffmpeg versions use AVIOInterruptCB
static int
media_interrupt_cb(void)
{
	int ret;
	pthread_mutex_lock(&mutex_interrupt);
	ret = interrupt;
	pthread_mutex_unlock(&mutex_interrupt);
	return ret;
}

void
set_interrrupt_cb(int i)
{
	pthread_mutex_lock(&mutex_interrupt);
	interrupt = i;
	pthread_mutex_unlock(&mutex_interrupt);
}

//TODO: ¿methods as synchronized?
Media::Media() throw(MediaException)
{
	LOG_TAG = "media";
	if(!initialized) {
		av_log_set_callback(media_av_log);
		av_register_all();
		if (av_lockmgr_register(lockmgr) !=0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't init media");
			throw MediaException("Couldn't init media");
		}
		avio_set_interrupt_cb(media_interrupt_cb);
		initialized = 1;
	}
	set_interrrupt_cb(0);
}

Media::~Media()
{

}
