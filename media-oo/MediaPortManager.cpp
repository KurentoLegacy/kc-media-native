
#include "MediaPortManager.h"

using namespace media;

MediaPort*
MediaPortManager::takeMediaPort()
{
	return new MediaPort();
}

MediaPort*
MediaPortManager::takeMediaPort(int port)
{
	return new MediaPort(port);
}

MediaPort*
MediaPortManager::takeMediaPort(const char* address, int port)
{
	return new MediaPort(address, port);
}

int
MediaPortManager::releaseMediaPort(MediaPort *mediaPort)
{
	int ret;

	if ((ret = mediaPort->close()) == 0)
		delete mediaPort;
	return ret;
}

