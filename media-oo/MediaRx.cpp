
#include "MediaRx.h"

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
