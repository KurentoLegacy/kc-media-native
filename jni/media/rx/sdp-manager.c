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

#include "sdp-manager.h"
#include "libavformat/rtsp.h"
#include "libavformat/rtpenc_chain.h"
#include "libavformat/internal.h"
#include "libavformat/rdt.h"

#include "socket-manager.h"

//Based on libavformat/rtsp.c
static int rtsp_open_transport_ctx(AVFormatContext *s, RTSPStream *rtsp_st)
{
	RTSPState *rt = s->priv_data;
	AVStream *st = NULL;
	
	/* open the RTP context */
	if (rtsp_st->stream_index >= 0)
		st = s->streams[rtsp_st->stream_index];
	if (!st)
		s->ctx_flags |= AVFMTCTX_NOHEADER;
	
	if (s->oformat && CONFIG_RTSP_MUXER) {
		rtsp_st->transport_priv = ff_rtp_chain_mux_open(s, st,
						rtsp_st->rtp_handle,
						RTSP_TCP_MAX_PACKET_SIZE);
		/* Ownership of rtp_handle is passed to the rtp mux context */
		rtsp_st->rtp_handle = NULL;
	} else if (rt->transport == RTSP_TRANSPORT_RDT && CONFIG_RTPDEC)
		rtsp_st->transport_priv = ff_rdt_parse_open(s, st->index,
						rtsp_st->dynamic_protocol_context,
						(RTPDynamicProtocolHandler*)rtsp_st->dynamic_handler);
	else if (CONFIG_RTPDEC)
		rtsp_st->transport_priv = rtp_parse_open(s, st, rtsp_st->rtp_handle,
							rtsp_st->sdp_payload_type,
			(rt->lower_transport == RTSP_LOWER_TRANSPORT_TCP || !s->max_delay)
			? 0 : RTP_REORDER_QUEUE_DEFAULT_SIZE);

	if (!rtsp_st->transport_priv) {
		return AVERROR(ENOMEM);
	} else if (rt->transport != RTSP_TRANSPORT_RDT && CONFIG_RTPDEC) {
		if (rtsp_st->dynamic_handler) {
			rtp_parse_set_dynamic_protocol(rtsp_st->transport_priv,
							rtsp_st->dynamic_protocol_context,
							rtsp_st->dynamic_handler);
		}
	}
	
	return 0;
}

//Based on libavformat/rtsp.c
static int my_sdp_read_header(AVFormatContext *s, AVFormatParameters *ap, const char *sdp_str)
{
	RTSPState *rt = s->priv_data;
	RTSPStream *rtsp_st;
	int size, i, err;
	char *content;
	char url[1024];
	
	if (!ff_network_init())
		return AVERROR(EIO);
	
	content = (char*)sdp_str;
	size = strlen(sdp_str);
	if (size <= 0)
		return AVERROR_INVALIDDATA;
	
	err = ff_sdp_parse(s, content);
	if (err) goto fail;

	/* open each RTP stream */
	for (i = 0; i < rt->nb_rtsp_streams; i++) {
		rtsp_st = rt->rtsp_streams[i];
		ff_url_join(url, sizeof(url), "rtp", NULL,
				"", rtsp_st->sdp_port,
				"?localport=%d&ttl=%d", rtsp_st->sdp_port,
				rtsp_st->sdp_ttl);
		
		URLContext *urlContext = get_connection_by_local_port(rtsp_st->sdp_port);
		if (urlContext)
			rtsp_st->rtp_handle = urlContext;
		else if ( (err = ffurl_open(&rtsp_st->rtp_handle, url, AVIO_RDWR)) < 0)
 			goto fail;
		if ((err = rtsp_open_transport_ctx(s, rtsp_st)))
			goto fail;
	}
	return 0;
	
fail:
	ff_rtsp_close_streams(s);
	ff_network_close();
	return err;
}

//Based on libavformat/utils.c
static int my_av_open_input_stream(AVFormatContext **ic_ptr, const char *sdp_str, AVInputFormat *fmt, AVFormatParameters *ap)
{
	int err;
	AVFormatContext *ic;
	AVFormatParameters default_ap;
	
	if(!ap){
		ap=&default_ap;
		memset(ap, 0, sizeof(default_ap));
	}

	if(!ap->prealloced_context)
		ic = avformat_alloc_context();
	else
		ic = *ic_ptr;
	if (!ic) {
		err = AVERROR(ENOMEM);
		goto fail;
	}
	ic->iformat = fmt;
	ic->duration = AV_NOPTS_VALUE;
	ic->start_time = AV_NOPTS_VALUE;
	
	/* allocate private data */
	if (fmt->priv_data_size > 0) {
		ic->priv_data = av_mallocz(fmt->priv_data_size);
		if (!ic->priv_data) {
			err = AVERROR(ENOMEM);
			goto fail;
 		}
	} else {
		ic->priv_data = NULL;
	}
	
	err = my_sdp_read_header(ic, ap, sdp_str);
	if (err < 0)
		goto fail;

    ic->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;

    *ic_ptr = ic;
    return 0;
    
fail:
	if (ic) {
		int i;
		av_freep(&ic->priv_data);
		for(i=0;i<ic->nb_streams;i++) {
			AVStream *st = ic->streams[i];
			if (st) {
				av_free(st->priv_data);
				av_free(st->codec->extradata);
				av_free(st->codec);
				av_free(st->info);
			}
			av_free(st);
		}
	}
	av_free(ic);
	*ic_ptr = NULL;
	return err;
}

//Based on libavformat/utils.c
int av_open_input_sdp(AVFormatContext **ic_ptr, const char *sdp_str, AVFormatParameters *ap)
{
	int err;
	AVInputFormat *fmt = NULL;
	
	fmt = av_find_input_format("sdp");
	
	/* if still no format found, error */
	if (!fmt) {
		err = AVERROR_INVALIDDATA;
		goto fail;
	}
	
	err = my_av_open_input_stream(ic_ptr, sdp_str, fmt, ap);
	if (err)
		goto fail;
	return 0;
fail:
	if (ap && ap->prealloced_context)
		av_free(*ic_ptr);
	*ic_ptr = NULL;
	return err;
}


