
#include "utils.h"
#include "libavformat/rtsp.h"

int64_t
get_pts(int64_t time, AVRational clock_rate)
{
	return (time * clock_rate.den) / (clock_rate.num * 1000);
}

int
get_local_port(URLContext *urlContext)
{
	return rtp_get_local_rtp_port(urlContext);
}

void
close_context(AVFormatContext *s)
{
	RTSPState *rt;
	RTSPStream *rtsp_st;
	int i;

	if (!s)
		return;

	// if is output
	if (s->oformat && s->pb) {
		s->pb->opaque = NULL;
		if (!(s->oformat->flags & AVFMT_NOFILE))
			avio_close(s->pb);
		av_free(s);
	}

	// if is input
	if (s->iformat && s->priv_data) {
		rt = (RTSPState*)(s->priv_data);
		for (i = 0; i < rt->nb_rtsp_streams; i++) {
			rtsp_st = (RTSPStream*)(rt->rtsp_streams[i]);
			rtsp_st->rtp_handle = NULL;
		}
		av_close_input_file(s);
	}
}
