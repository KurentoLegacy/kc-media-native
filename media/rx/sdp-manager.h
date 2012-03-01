#ifndef __SDP_MANAGER_H__
#define __SDP_MANAGER_H__
 
#include "libavformat/avformat.h"

int av_open_input_sdp(AVFormatContext **ic_ptr, const char *sdp_str,
								AVFormatParameters *ap);

#endif /* __SDP_MANAGER_H__ */