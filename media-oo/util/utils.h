
#ifndef __UTILS_H__
#define __UTILS_H__

#include <libavformat/avformat.h>

int get_local_port(URLContext *urlContext);
void close_context(AVFormatContext *s);

#endif /* __UTILS_H__ */
