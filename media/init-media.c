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

#include "init-media.h"
#include <util/log.h>
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int initialized = 0;

/*
	see	libavutil/log.c
		ffserver.c
*/

static void
media_av_log(void *ptr, int level, const char *fmt, va_list vargs)
{
	int media_log_level =  MEDIA_LOG_UNKNOWN;

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
lockmgr(void **mtx, enum AVLockOp op) {
	switch(op) {
	case AV_LOCK_CREATE:
		*mtx = malloc(sizeof(pthread_mutex_t));
		if(!*mtx)
			return 1;
		return !!pthread_mutex_init(*mtx, NULL);
	case AV_LOCK_OBTAIN:
		return !!pthread_mutex_lock(*mtx);
	case AV_LOCK_RELEASE:
		return !!pthread_mutex_unlock(*mtx);
	case AV_LOCK_DESTROY:
		pthread_mutex_destroy(*mtx);
		free(*mtx);
		return 0;
	}
	return 1;
}

int
init_media() {
	int ret = 0;
	
	pthread_mutex_lock(&mutex);
	if(!initialized) {
		av_log_set_callback(media_av_log);
		av_register_all();
		ret = av_lockmgr_register(lockmgr);
		initialized++;
	}
	pthread_mutex_unlock(&mutex);
	
	return ret;
}