
#include "MediaRx.h"

extern "C" {
#include <socket-manager.h>
#include "sdp-manager.h"
}

using namespace media;

MediaRx::MediaRx(const char* sdp, int max_delay)
: Media()
{
	_sdp = sdp;
	_max_delay = max_delay;
	_mutex = new Lock();
	_freeLock = new Lock();
	LOG_TAG = "media-rx";
}

MediaRx::~MediaRx()
{
	delete _mutex;
	delete _freeLock;
}

bool
MediaRx::getReceive()
{
	bool ret;
	_mutex->lock();
	ret = _receive;
	_mutex->unlock();

	return ret;
}

void
MediaRx::setReceive(bool receive)
{
	_mutex->lock();
	_receive = receive;
	_mutex->unlock();
}

int
MediaRx::openFormatContext(AVFormatContext **c)
{
	AVFormatContext *pFormatCtx = NULL;
	AVFormatParameters params, *ap = &params;
	int ret;
	char buf[256];

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sdp: %s", _sdp);

	for(;;) {
		if (!this->getReceive())
			return -1;

		pFormatCtx = avformat_alloc_context();
		pFormatCtx->max_delay = _max_delay * 1000;
		ap->prealloced_context = 1;

		// Open sdp
		if ((ret = av_open_input_sdp(&pFormatCtx, _sdp, ap)) != 0 ) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't process sdp: %s", buf);
			return ret;
		}

		// Retrieve stream information
		if ((ret = av_find_stream_info(pFormatCtx)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_WARN, LOG_TAG, "Couldn't find stream information: %s", buf);
			close_context(pFormatCtx);

		} else
			break;
	}
	*c = pFormatCtx;

	return 0;
}
