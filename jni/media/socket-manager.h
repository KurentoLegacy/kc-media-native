/**
 * 
 * @author Miguel París Díaz
 * 
 */
 
#include "libavformat/avformat.h"

void close_context(AVFormatContext *s);
URLContext* get_connection_by_local_port(int local_port);
URLContext* get_audio_connection(int audioPort);
URLContext* get_video_connection(int videoPort);

