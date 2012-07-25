
#include "MediaPortManager.h"

using namespace media;

MediaPort*
MediaPortManager::createMediaPort()
{
	return new MediaPort();
}

MediaPort*
MediaPortManager::createMediaPort(int port)
{
	return new MediaPort(port);
}

int
MediaPortManager::releaseMediaPort(MediaPort *mediaPort)
{
	int ret;

	if ((ret = mediaPort->close()) == 0)
		delete mediaPort;
	return ret;
}
