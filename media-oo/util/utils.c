
#include "utils.h"
#include "libavformat/rtsp.h"

int
get_local_port(URLContext *urlContext)
{
	return rtp_get_local_rtp_port(urlContext);
}

void
close_input(AVFormatContext *s)
{
	RTSPState *rt;
	RTSPStream *rtsp_st;
	int i;

	if (!s)
		return;

	if (s->iformat && s->priv_data) {
		rt = (RTSPState*)(s->priv_data);
		for (i = 0; i < rt->nb_rtsp_streams; i++) {
			rtsp_st = (RTSPStream*)(rt->rtsp_streams[i]);
			rtsp_st->rtp_handle = NULL;
		}
		av_close_input_file(s);
	}
}
