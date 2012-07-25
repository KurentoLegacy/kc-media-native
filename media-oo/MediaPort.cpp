
#include "MediaPort.h"

extern "C" {
#include "util/utils.h"
}

using namespace media;

MediaPort::MediaPort()
{
	LOG_TAG = "media-port";
	_formatContext = open("rtp://0.0.0.0:0");
	_n_users = 1;
	_mutex = new Lock();
}

MediaPort::MediaPort(int port)
{
	char rtp[256];

	LOG_TAG = "media-port";
	snprintf(rtp, sizeof(rtp), "rtp://0.0.0.0?localport=%d", port);
	_formatContext = open(rtp);
	_n_users = 1;
	_mutex = new Lock();
}

MediaPort::~MediaPort()
{
	delete _mutex;
}

int
MediaPort::getPort()
{
	int port;

	_mutex->lock();
	port = get_local_port(_urlContext);
	_mutex->unlock();

	return port;
}

URLContext*
MediaPort::getConnection()
{
	URLContext* urlContext;

	_mutex->lock();
	urlContext = _urlContext;
	_n_users++;
	_mutex->unlock();

	return urlContext;
}

void
MediaPort::closeContext(AVFormatContext *s)
{
//	RTSPState *rt;
//	RTSPStream *rtsp_st;
//	int i;

	if (!s)
		return;

	// if is output
	if (s->oformat && s->pb) { //  && (s->pb->opaque == _urlContext)) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free output");
		//free_connection(s->pb->opaque);
//		this->close();
		s->pb->opaque = NULL;
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "closeContext %d s: %p s->pb: %p", __LINE__, s, s->pb);
		if (!(s->oformat->flags & AVFMT_NOFILE))
			avio_close(s->pb);
		av_free(s);
	}

	// if is input
	if (s->iformat && s->priv_data) {
/*		rt = (RTSPState*)(s->priv_data);
		for (i = 0; i < rt->nb_rtsp_streams; i++) {
			rtsp_st = (RTSPStream*)(rt->rtsp_streams[i]);
			media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free input");
			if (rtsp_st->rtp_handle == _urlContext)
				//free_connection(rtsp_st->rtp_handle);
				this->close();
			rtsp_st->rtp_handle = NULL;
		}
*/
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "free input");
		close_input(s);
		//av_close_input_file(s);
	}
}

int
MediaPort::close()
{
	int ret;
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "close %d", __LINE__);
	_mutex->lock();
	if (--_n_users == 0) {
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "close %d _formatContext: %p _formatContext->pb: %p", __LINE__, _formatContext, _formatContext->pb);
		avio_close(_formatContext->pb);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "close %d", __LINE__);
		avformat_free_context(_formatContext);
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "close %d", __LINE__);
		_formatContext = NULL;
		_urlContext = NULL;
	}
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "close %d", __LINE__);
	ret = _n_users;
	_mutex->unlock();
media_log(MEDIA_LOG_DEBUG, LOG_TAG, "close %d", __LINE__);
	return ret;
}

AVFormatContext*
MediaPort::open(const char *rtp)
{
	AVFormatContext *s = NULL;
	char buf[256];
	int ret;

	s = avformat_alloc_context();
	if (!s) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG,
				"Memory error: Could not alloc context");
		s = NULL;
	} else if ((ret = avio_open(&s->pb, rtp, AVIO_RDWR)) < 0) {
		av_strerror(ret, buf, sizeof(buf));
		media_log(MEDIA_LOG_ERROR, LOG_TAG,
				"Could not open '%s': %s", rtp, buf);
		av_free(s);
		s = NULL;
	}

	if (s)
		_urlContext = (URLContext*)(s->pb->opaque);

	return s;
}
