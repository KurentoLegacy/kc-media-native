
#ifndef __MEDIA_PORT_MANAGER_H__
#define __MEDIA_PORT_MANAGER_H__

#include "Media.h"
#include "MediaPort.h"

namespace media {
	class MediaPortManager : public Media {
	public:
		static MediaPort* takeMediaPort();
		static MediaPort* takeMediaPort(int port);
		static int releaseMediaPort(MediaPort *mediaPort);
	};
}

#endif /* __MEDIA_PORT_MANAGER_H__ */