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
#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

#define LOG_BUF_SIZE 256

typedef enum _MediaLogLevel {
	MEDIA_LOG_UNKNOWN = 0,
	MEDIA_LOG_VERBOSE,
	MEDIA_LOG_DEBUG,
	MEDIA_LOG_INFO,
	MEDIA_LOG_WARN,
	MEDIA_LOG_ERROR,
	MEDIA_LOG_FATAL,
} MediaLogLevel;

void media_log(MediaLogLevel level, const char *tag, const char *fmt, ...);
void media_vlog(MediaLogLevel level, const char *tag, const char *fmt, va_list vl);
void media_log_set_callback(void (*callback)(MediaLogLevel level, const char *tag,
						const char *fmt, va_list vargs));

#endif /* __LOG_H__ */
