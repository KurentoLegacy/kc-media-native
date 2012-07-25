
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
	close_context(s);
}

int
MediaPort::close()
{
	int ret;

	_mutex->lock();
	if (--_n_users == 0) {
		avio_close(_formatContext->pb);
		avformat_free_context(_formatContext);
		_formatContext = NULL;
		_urlContext = NULL;
	}
	ret = _n_users;
	_mutex->unlock();

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
