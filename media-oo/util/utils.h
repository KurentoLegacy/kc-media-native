
#ifndef __UTILS_H__
#define __UTILS_H__

#include <libavformat/avformat.h>
#include "libavutil/rational.h"

int64_t get_pts(int64_t time, AVRational clock_rate);

int get_local_port(URLContext *urlContext);
void close_context(AVFormatContext *s);

#endif /* __UTILS_H__ */
