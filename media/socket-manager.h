#ifndef __SOCKET_MANAGER_H__
#define __SOCKET_MANAGER_H__
 
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

void close_context(AVFormatContext *s);
URLContext* get_connection_by_local_port(int local_port, enum AVMediaType media_type);
URLContext* get_audio_connection(int audioPort);
URLContext* get_video_connection(int videoPort);

int take_audio_local_port(int audioPort);
int release_audio_local_port();
int take_video_local_port(int videoPort);
int release_video_local_port();

#endif /* __SOCKET_MANAGER_H__ */
