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
